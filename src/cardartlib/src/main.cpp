#include "manalink.h"
#include "CardArtLib.h"

#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>

namespace gdi = Gdiplus;
namespace fs = boost::filesystem;

static std::unordered_map<std::string, std::unique_ptr<gdi::Image>> *cache;
static char** idToName;
static size_t numberOfCards = 0;
static CRITICAL_SECTION crit;
//gdiplus
static ULONG_PTR gdiplusToken;
static ULONG_PTR gdiplusBGThreadToken;
static gdi::GdiplusStartupInput gdiplusStartupInput;
static gdi::GdiplusStartupOutput gdiplusStartupOutput;
static bool images_counted = false;

static card_ptr_t* cards_ptr_shandalar_dll = NULL;

static std::unique_ptr<wchar_t[]> stringToWChar(const std::string& s)
{
	int len = s.length() + 1;
	std::unique_ptr<wchar_t[]> wcstring(new wchar_t[len]);
	mbstowcs(wcstring.get(), s.c_str(), len);
	return wcstring;
}

static void loadDat(const char* fname){
	bool okchar[256] = {0};
	for (int i = 'a'; i <= 'z'; ++i)
		okchar[i] = true;
	for (int i = 'A'; i <= 'Z'; ++i)
		okchar[i] = true;
	for (int i = '0'; i <= '9'; ++i)
		okchar[i] = true;
	okchar[','] = true;
	okchar[' '] = true;
	okchar['\''] = true;
	okchar['-'] = true;

	FILE* f = fopen(fname, "rb");
	size_t stringDataSize;
	fread(&numberOfCards, 4, 1, f);
	fread(&stringDataSize, 4, 1, f);
	char* stringData = (char*)malloc(stringDataSize);
	card_ptr_t* cardData = (card_ptr_t*)malloc(sizeof(card_ptr_t)*numberOfCards);
	fread(cardData, sizeof(card_ptr_t), numberOfCards, f);
	fread(stringData, 1, stringDataSize, f);
	fclose(f);
	idToName = (char**)calloc(numberOfCards, sizeof(char*));

	for (unsigned int i = 0; i < numberOfCards; i++) {
		int id = cardData[i].id;//*((int*)cardData + 38*i);
		char* name = (stringData + (int)(cardData[i].full_name));//*((int*)cardData + 1 + 38*i));
		//replace characters
		for (char* p = name; *p; ++p)
		  if (!okchar[(int)(unsigned char)(*p)])
			*p = '_';
		idToName[id] = (char*)malloc(strlen(name) + 1);
		strcpy(idToName[id], name);
	}
	free(cardData);
	free(stringData);
}

static void load_names(void)
{
	// Precondition: idToName == NULL

	HMODULE parent = GetModuleHandle(0);

	if (GetProcAddress(parent, "szDeckName")) { //manalink
		char* cards_dat = (char*)0x4D722E;
		loadDat(cards_dat);
	} else if (GetModuleHandle("deck.exe")) { //deck.exe
		loadDat("Cards.dat");
	} else { // shandalar
		loadDat("Cards.dat");
	}
}

static std::string idToNameFun(int id, int version) {
	if (idToName == NULL){
		load_names();
	}
	char tmp[254];
	if (version > 0) {
		sprintf(tmp, "CardArtManalink/%s (%d).jpg", idToName[id], version);
	} else {
		sprintf(tmp, "CardArtManalink/%s.jpg", idToName[id]);
	}

	std::string path = tmp;
	return path;
}

static void count_images(void)
{
	// Precondition: images_counted == false
	// Precondition: within critical section &crit

	images_counted = true;

	card_ptr_t* card_ptrs = NULL;

	HMODULE parent = GetModuleHandle(0);

	if (GetProcAddress(parent, "szDeckName")) { //manalink
		card_ptrs = *((card_ptr_t**)(0x73bae0));
	} else if (GetModuleHandle("deck.exe")) { //deck.exe
		// No need to do anything; deckbuilder always shows first image only
		return;
	} else { // shandalar
		if (!cards_ptr_shandalar_dll)
			return;
		card_ptrs = cards_ptr_shandalar_dll;
	}

	if (idToName == NULL)
		load_names();

	std::unordered_map<std::string, int> name_to_csvid;
	char buf[200];	// Manalink itself will crash for names much shorter than this
	for (unsigned int i = 0; i < numberOfCards; ++i) {
		const char* name = idToName[i];
		for (char* p = buf; (*p = tolower(*name)); ++p, ++name){}
		name_to_csvid[buf] = i;
		card_ptrs[i].num_pics = 1;	// Always assume at least one image; it would have been corrected to 1 if initially set to 0 anyway
	}

	for (fs::directory_iterator dirit(fs::path("./CardArtManalink")), end; dirit != end; ++dirit){
		try {
			if (!is_regular_file(dirit->status()))
				continue;

			std::string filename = dirit->path().filename().string();

			/* boost::regex would be more concise, but this is about 10% faster when not disk-bound, and more importantly, doesn't bloat the dll file size.
			 * boost::filesystem already increases it from 328k to 1040k; adding boost::regex ballooned it to 1540k. */
			char cardname[200];
			const char* name = filename.c_str();
			unsigned int num = 0;
			for (char* p = cardname; (*p = tolower(*name)); ++p, ++name){
				if (*p == '('){
					if (p > cardname && *(name - 1) == ' ' && *(name + 1) >= '1' && *(name + 1) <= '9'){
						*(p - 1) = 0;	// overwriting the last space before the parenthesis
						++name;
						num = atoi(name);
						while (*name && *name >= '0' && *name <= '9')
							++name;
						if (strcmp(name, ").jpg"))
							num = 0;	// didn't match epilogue
						break;
					} else
						break;	// ( found, but not after a space, or not followed by [1..9]
				}
			}
			if (num == 0)	// no image number found, or failed after finding (
				continue;

			std::unordered_map<std::string, int>::const_iterator csvidit = name_to_csvid.find(cardname);
			if (csvidit == name_to_csvid.end())
				continue;

			int csvid = csvidit->second;
			++num;	// "Grizzly Bears (3).jpg" is actually the fourth image
			if (card_ptrs[csvid].num_pics < num)
				card_ptrs[csvid].num_pics = num;
		} catch (...) {}	// can't read dir, file stopped existing between iteration and status, etc. - we don't care, just skip it.
	}
}

void Cardartlib_initialize_for_shandalar(card_data_t* real_cards_data, card_ptr_t* real_cards_ptr)
{
	// Avoid querying Shandalar.dll directly; it may not be present.
	(void)real_cards_data;
	cards_ptr_shandalar_dll = real_cards_ptr;
	count_images();	// during startup instead of waiting for first to display
}

static bool file_exists(const char *filename)
{
	std::unique_ptr<wchar_t[]> wFilename = stringToWChar(filename);
	if (_waccess(wFilename.get(), 4) == 0) // check read permission
		return true;
	return false;
}

int LoadBigArt(int id, int version, LONG width, int height) {
	EnterCriticalSection(&crit);
	if (!images_counted)
		count_images();
	std::string path = idToNameFun(id, version);
	std::unordered_map<std::string, std::unique_ptr<gdi::Image>>::const_iterator it = cache->find(path);
	if (it == cache->end()) {
		if (!file_exists(path.c_str())) {
			LeaveCriticalSection(&crit);
			return 0;
		}
		std::unique_ptr<wchar_t[]> wPath = stringToWChar(path);
		std::unique_ptr<gdi::Image> tmpSurface(new gdi::Image(wPath.get()));
		(*cache)[path] = move(tmpSurface);
	}
	LeaveCriticalSection(&crit);
	return 1; // 1 if the art is available
}

int LoadSmallArt(int id, int version, LONG width, int height) {
	return LoadBigArt(id, version, width, height);
}

void DestroyBigArt(int id, int version) {
	EnterCriticalSection(&crit);
	std::string path = idToNameFun(id, version);
	std::unordered_map<std::string, std::unique_ptr<gdi::Image>>::iterator it = cache->find(path);
	if (it != cache->end()) {
		cache->erase(it);
		//MessageBox(0, path.c_str(), 0, MB_ICONSTOP | MB_SYSTEMMODAL);
	}
	LeaveCriticalSection(&crit);
} //Shandalar

void DestroySmallArt(int id, int version) {
	DestroyBigArt(id, version);
} //Shandalar

int IsBigArtRightSize(int id, int version, int width, int height) {
	return LoadBigArt(id, version, width, height);
}

int IsSmallArtRightSize(int id, int version, int width, int height) {
	return LoadBigArt(id, version, width, height);
}

void DestroyAllBigArts(void) {
	EnterCriticalSection(&crit);
	cache->clear();
	LeaveCriticalSection(&crit);
} //Shandalar

void DestroyAllSmallArts(void) {
	DestroyAllBigArts();
} //Shandalar

int IsBigArtIn(int id, int version) {
	return 0;
}

int IsSmallArtIn(int id, int version) {
	return 0;
}

int ReloadBigArtIfWrongSize(int id, int version, LONG width, int height) {
	return 0;
} //Shandalar

int ReloadSmallArtIfWrongSize(int id, int version) {
	return 0;
}

int DrawBigArt(HDC hdc, const RECT* rect, int id, int version) {
	if (!LoadBigArt(id, version, 0, 0)) {
		EnterCriticalSection(&crit);
		gdi::Graphics graphics(hdc);
		gdi::SolidBrush brush(gdi::Color(211,211,211));
		gdi::Rect tmpRect(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
		graphics.FillRectangle(&brush, tmpRect);
		LeaveCriticalSection(&crit);
		return 0;
	}
	EnterCriticalSection(&crit);
	gdi::Graphics graphics(hdc);

	std::string path = idToNameFun(id, version);
	gdi::Rect tmpRect(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top);
	graphics.DrawImage((*cache)[path].get(), tmpRect);

	LeaveCriticalSection(&crit);
	return 1;
}

int DrawSmallArt(HDC hdc, const RECT* rect, int id, int version) {
	return DrawBigArt(hdc, rect, id, version);
}

static void init() {
	cache = new std::unordered_map<std::string, std::unique_ptr<gdi::Image>>;

	InitializeCriticalSection(&crit);
	gdiplusStartupInput.SuppressBackgroundThread = TRUE;
	GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, &gdiplusStartupOutput);
	gdi::Status stat = gdiplusStartupOutput.NotificationHook(&gdiplusBGThreadToken);
	assert(stat == gdi::Ok);

	images_counted = false;
}

static void deinit() {
	delete cache;
	if (idToName) {
		for (unsigned int i = 0; i < numberOfCards; ++i)
			if (idToName[i])
				free(idToName[i]);
		free(idToName);
	}

	DeleteCriticalSection(&crit);
	gdiplusStartupOutput.NotificationUnhook(gdiplusBGThreadToken);
	gdi::GdiplusShutdown(gdiplusToken);
}

int WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID reserved) {
	switch (nReason)
	{
	case DLL_PROCESS_ATTACH:
		init();
		return TRUE;
		break;
	case DLL_PROCESS_DETACH:
		DestroyAllBigArts();
		DestroyAllSmallArts();
		deinit();
		return TRUE;
		break;
	default:
		return TRUE;
	}
}
