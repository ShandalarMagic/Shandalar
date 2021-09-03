package gui;

import javax.swing.JOptionPane;

/**
 * A class used to represent a card from the Shandalar game.
 * @author Ryan
 */
public class gameCard {

    private String id;   // when read from game this contains 8 chars first 4 for id then 4 for deck
    private String deck; // No Deck = 00 Deck 1 = 01 Deck 2 = 02 Deck 3 = 04 add them together
                         // For more than 1 deck (it's done with bit flags so > 7 = No deck)  
    private String name;    
    private int quantity;
    
    static String nl = System.getProperty("line.separator");    
    
    /**
     * Empty constructor initiates the variables
     */
    public gameCard(){
        id = "0000";
        deck = "00";
        name = "";
        quantity = 1;
    }
    
    /**
     * Two String constructor sets the card's id and name to the given 
     * argurments. Quantity is set to 1 as cards in the actual game file
     * don't store quantity, we do so we know how many times to loop writing
     * it to the file.
     * @param id The hexidecimal game code of the card.
     * @param name The name of the card.
     */
    public gameCard(String id, String name){
    this.id=id;
    this.name = name;
    deck = "00";
    quantity = 1;
    }

    /**
     * Three String constructor sets the card's id, name and deck to the given 
     * argurments. Quantity is set to 1 as cards in the actual game file
     * don't store quantity, we do so we know how many times to loop writing
     * it to the file.
     * @param id The hexidecimal game code of the card.
     * @param name The name of the card.
     * @param deck Hexidecimal number of the decks this card is in.
     */
    public gameCard(String id, String name, String deck){
    this.id=id;
    this.name = name;
    this.deck = deck;
    quantity = 1;
    }
    
    // <editor-fold defaultstate="collapsed" desc="Getters and setters">
    /**
     * Get the game card's id.
     * @return String Hexidecimal representation of the card's game code.
     */
    public String getID(){return this.id;}
    /**
     * Set the game card's id
     * @param id String Hexadecimal representation of the card's game code.
     */
    public void setID(String id){this.id=id;}
    /**
     * Gets the game card's name
     * @return String The name of the card
     */
    public String getName(){return this.name;}
    /**
     * Sets the game card's name
     * @param name String The name of the card
     */
    public void setName(String name){this.name=name;}
    /**
     * Gets the game card's deck.
     * @return String The hexadecimal representation of the decks the card is in.
     */
    public String getDeck(){return this.deck;}
    /**
     * Sets the game card's deck.
     * @param name String The hexadecimal representation of the decks the card is in.
     */
    public void setDeck(String deck){this.deck=deck;}
    /**
     * Sets the deck the card is a part of using an int for the deck number (0,1,2 or 3)
     * and a boolean to indicate whether or not the card should be kept a part of 
     * the other decks it may be used in.
     * @param deck An int for the actually deck number.
     * @param keepCommitments Boolean indicating if the card should be left in the other
     * decks it is a part of.
     */
    public void setDeck(int deck, boolean keepCommitments) {
        if (!keepCommitments){ // we don't want it to be a part of other decks so just
        switch (deck){         // set the deck number to the one given.
            case 0: this.deck = "00";
                    break;
            case 1: this.deck = "01";
                    break;
            case 2: this.deck = "02";
                    break;
            case 3: this.deck = "04";            
                    break;
            case 4: this.deck = "04";
                    break;
            default: this.deck = "00";
        }        
        }else{ // we do want to keep the other decks so OR the new deck number with the old one.
            int deckNo = Integer.parseInt(this.deck.substring(0, 2));
            deckNo = deckNo | deck;
            this.deck = "0" + String.valueOf(deckNo);
        }
    }
    /**
     * Removes the card from the given deck
     * @param deck The deck to remove the card from
     */
    public void unSetDeck(int deck) {
        // to unset we AND the current deck number with the inverse of the one to remove it from
        int deckNo = Integer.parseInt(this.deck.substring(0, 2));
            deckNo = deckNo & ~deck;
            this.deck = "0" + String.valueOf(deckNo);
    }
    /**
     * Gets the game card's quantity. The quantity is used to mark how many times the system
     * should loop round writing the card data to the save.
     * @return int The number of times this card appears in the deck.
     */
    public int getQuantity(){return this.quantity;}
    /**
     * Sets the game card's quantity. The quantity is used to mark how many times the system
     * should loop round writing the card data to the save.
     * @param quantity int The number of times this card appears in the deck.
     */
    public void setQuantity(int quantity){this.quantity=quantity;}
// </editor-fold>
    
    /**
     * This method is used to convert a game card into a duel card.
     * We do so by taking a map of how the two cards are joined and
     * then getting the codePair that matches the name of the card.
     * From this we can get the new code and the the quantity is 
     * just copied across. 
     * @param cardMap The mapping of card names to card codes.
     * @return duelCard A duel card representation of the game card.
     */
    public duelCard convert(allCards cardMap) {
        try {
            duelCard outCard = new duelCard();
            outCard.setName(name);
            int newID = cardMap.gameToDuel(id.substring(0, 4));
            if (newID == -1) {
                return null;
            }
            outCard.setID(newID);
            outCard.setQuantity(quantity);
            return outCard;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst converting from game card code to duel card code in gameCard class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }
    
    /**
     * Creates a duplicate of this card.
     * @return A duplicate of this card.
     */
    @Override
    public gameCard clone() {
     gameCard gC = new gameCard(id, name, deck); 
     gC.setQuantity(quantity);
     return gC;
    }

    /**
     * There is a problem when reading cards that depending on the deck the card
     * is in the code of the card changes. This method eliminates that problem by
     * zero'ing out the number that changes.
     * @return The card we just fixed.
     */
    public gameCard fix() {
        id = id.substring(0, 2) + "0" + id.substring(3);
        return this;
    }
}

