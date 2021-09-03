#define WINGDIPAPI __stdcall
#define GDIPCONST const

typedef enum
{
  Ok = 0,
  GenericError = 1,
  InvalidParameter = 2,
  OutOfMemory = 3,
  ObjectBusy = 4,
  InsufficientBuffer = 5,
  NotImplemented = 6,
  Win32Error = 7,
  WrongState = 8,
  Aborted = 9,
  FileNotFound = 10,
  ValueOverflow = 11,
  AccessDenied = 12,
  UnknownImageFormat = 13,
  FontFamilyNotFound = 14,
  FontStyleNotFound = 15,
  NotTrueTypeFont = 16,
  UnsupportedGdiplusVersion = 17,
  GdiplusNotInitialized = 18,
  PropertyNotFound = 19,
  PropertyNotSupported = 20
} Status;

typedef enum
{
  QualityModeInvalid	= -1,
  QualityModeDefault	= 0,
  QualityModeLow		= 1, // Best performance
  QualityModeHigh		= 2  // Best rendering quality
} QualityMode;

typedef enum
{
  InterpolationModeInvalid		= QualityModeInvalid,
  InterpolationModeDefault		= QualityModeDefault,
  InterpolationModeLowQuality	= QualityModeLow,
  InterpolationModeHighQuality	= QualityModeHigh,
  InterpolationModeBilinear,
  InterpolationModeBicubic,
  InterpolationModeNearestNeighbor,
  InterpolationModeHighQualityBilinear,
  InterpolationModeHighQualityBicubic
} InterpolationMode;

typedef enum
{
  UnitWorld,		// 0 -- World coordinate (non-physical unit)
  UnitDisplay,		// 1 -- Variable -- for PageTransform only
  UnitPixel,		// 2 -- Each unit is one device pixel.
  UnitPoint,		// 3 -- Each unit is a printer's point, or 1/72 inch.
  UnitInch,			// 4 -- Each unit is 1 inch.
  UnitDocument,		// 5 -- Each unit is 1/300 inch.
  UnitMillimeter	// 6 -- Each unit is 1 millimeter.
} Unit;

typedef enum
{
	DebugEventLevelFatal,
	DebugEventLevelWarning
} DebugEventLevel;

typedef enum
{
  RotateNoneFlipNone = 0,
  Rotate90FlipNone   = 1,
  Rotate180FlipNone  = 2,
  Rotate270FlipNone  = 3,

  RotateNoneFlipX    = 4,
  Rotate90FlipX      = 5,
  Rotate180FlipX     = 6,
  Rotate270FlipX     = 7,

  RotateNoneFlipY    = Rotate180FlipX,
  Rotate90FlipY      = Rotate270FlipX,
  Rotate180FlipY     = RotateNoneFlipX,
  Rotate270FlipY     = Rotate90FlipX,

  RotateNoneFlipXY   = Rotate180FlipNone,
  Rotate90FlipXY     = Rotate270FlipNone,
  Rotate180FlipXY    = RotateNoneFlipNone,
  Rotate270FlipXY    = Rotate90FlipNone
} RotateFlipType;

typedef enum
{
	ImageLockModeRead			= 0x0001,
	ImageLockModeWrite			= 0x0002,
	ImageLockModeUserInputBuf	= 0x0004
} ImageLockMode;

typedef enum
{
	ColorMatrixFlagsDefault		= 0,
	ColorMatrixFlagsSkipGrays	= 1,
	ColorMatrixFlagsAltGray		= 2
} ColorMatrixFlags;

typedef enum
{
	ColorAdjustTypeDefault,
	ColorAdjustTypeBitmap,
	ColorAdjustTypeBrush,
	ColorAdjustTypePen,
	ColorAdjustTypeText,
	ColorAdjustTypeCount,
	ColorAdjustTypeAny		// Reserved
} ColorAdjustType;


typedef INT PixelFormat;
#define PixelFormatAlpha		0x00040000 // Has an alpha component
#define PixelFormatGDI			0x00020000 // Is a GDI-supported format
#define PixelFormatCanonical	0x00200000
#define PixelFormat32bppRGB		(9 | (32 << 8) | PixelFormatGDI)
#define PixelFormat32bppARGB	(10 | (32 << 8) | PixelFormatAlpha | PixelFormatGDI | PixelFormatCanonical)

typedef Status GpStatus;
typedef struct {} GpGraphics;
typedef struct {} GpImage;
typedef GpImage GpBitmap;
typedef Unit GpUnit;
typedef struct {} GpImageAttributes;
typedef BOOL (CALLBACK * ImageAbort)(VOID *);
typedef ImageAbort DrawImageAbort;
typedef VOID (WINAPI *DebugEventProc)(DebugEventLevel level, CHAR *message);
typedef Status (WINAPI *NotificationHookProc)(OUT ULONG_PTR *token);
typedef VOID (WINAPI *NotificationUnhookProc)(ULONG_PTR token);
typedef DWORD ARGB;
typedef float REAL;

typedef struct
{
	UINT32 GdiplusVersion;				// Must be 1
	DebugEventProc DebugEventCallback;	// Ignored on free builds
	BOOL SuppressBackgroundThread;		// FALSE unless you're prepared to call the hook/unhook functions properly
	BOOL SuppressExternalCodecs;		// FALSE unless you want GDI+ only to use its internal image codecs.
} GdiplusStartupInput;

typedef struct
{
	UINT Width;
	UINT Height;
	INT Stride;
	PixelFormat PixelFormat;
	VOID* Scan0;
	UINT_PTR Reserved;
} BitmapData;

typedef struct
{
	REAL m[5][5];
} ColorMatrix;

typedef struct
{
	INT X;
	INT Y;
	INT Width;
	INT Height;
} GpRect;

typedef struct
{
  // The following 2 fields are NULL if SuppressBackgroundThread is FALSE.
  // Otherwise, they are functions which must be called appropriately to
  // replace the background thread.
  //
  // These should be called on the application's main message loop - i.e.
  // a message loop which is active for the lifetime of GDI+.
  // "NotificationHook" should be called before starting the loop,
  // and "NotificationUnhook" should be called after the loop ends.

  NotificationHookProc NotificationHook;
  NotificationUnhookProc NotificationUnhook;
} GdiplusStartupOutput;

VOID WINAPI GdiplusShutdown(ULONG_PTR token);
Status WINAPI GdiplusStartup(OUT ULONG_PTR *token, const GdiplusStartupInput *input, OUT GdiplusStartupOutput *output);
GpStatus WINGDIPAPI GdipBitmapLockBits(GpBitmap* bitmap, GDIPCONST GpRect* rect, UINT flags, PixelFormat format, BitmapData* lockedBitmapData);
GpStatus WINGDIPAPI GdipBitmapUnlockBits(GpBitmap* bitmap, BitmapData* lockedBitmapData);
GpStatus WINGDIPAPI GdipCloneBitmapAreaI(INT x, INT y, INT width, INT height, PixelFormat format, GpBitmap *srcBitmap, GpBitmap **dstBitmap);
GpStatus WINGDIPAPI GdipCreateBitmapFromScan0(INT width, INT height, INT stride, PixelFormat format, BYTE* scan0, GpBitmap** bitmap);
GpStatus WINGDIPAPI GdipCreateFromHDC(HDC hdc, GpGraphics **graphics);
GpStatus WINGDIPAPI GdipCreateHBITMAPFromBitmap(GpBitmap* bitmap, HBITMAP* hbmReturn, ARGB background);
GpStatus WINGDIPAPI GdipCreateImageAttributes(GpImageAttributes **imageattr);
GpStatus WINGDIPAPI GdipDeleteGraphics(GpGraphics *graphics);
GpStatus WINGDIPAPI GdipDisposeImage(GpImage *image);
GpStatus WINGDIPAPI GdipDisposeImageAttributes(GpImageAttributes *imageattr);
GpStatus WINGDIPAPI GdipDrawImageI(GpGraphics *graphics, GpImage *image, INT x, INT y);
GpStatus WINGDIPAPI GdipDrawImageRectI(GpGraphics *graphics, GpImage *image, INT x, INT y, INT width, INT height);
GpStatus WINGDIPAPI GdipDrawImageRectRectI(GpGraphics *graphics, GpImage *image, INT dstx, INT dsty, INT dstwidth, INT dstheight,
										   INT srcx, INT srcy, INT srcwidth, INT srcheight,
										   GpUnit srcUnit, GDIPCONST GpImageAttributes* imageAttributes, DrawImageAbort callback, VOID* callbackData);
GpStatus WINGDIPAPI GdipGetImageGraphicsContext(GpImage *image, GpGraphics **graphics);
GpStatus WINGDIPAPI GdipGetImageHeight(GpImage *image, UINT *height);
GpStatus WINGDIPAPI GdipGetImageWidth(GpImage *image, UINT *width);
GpStatus WINGDIPAPI GdipImageRotateFlip(GpImage *image, RotateFlipType rfType);
GpStatus WINGDIPAPI GdipSetImageAttributesColorMatrix(GpImageAttributes *imageattr, ColorAdjustType type, BOOL enableFlag, GDIPCONST ColorMatrix* colorMatrix, GDIPCONST ColorMatrix* grayMatrix, ColorMatrixFlags flags);
GpStatus WINGDIPAPI GdipSetInterpolationMode(GpGraphics *graphics, InterpolationMode interpolationMode);
