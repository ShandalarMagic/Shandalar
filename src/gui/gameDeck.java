package gui;

import java.util.ArrayList;
import java.util.Iterator;
import javax.swing.JOptionPane;

/**
 * A store for game cards. Over-rides the ArrayList's to String method
 * to make one that returns the name and id of the items held.
 * @author Ryan
 */
public class gameDeck extends ArrayList<gameCard> {

     static String nl = System.getProperty("line.separator");
    
    /**
     * A String representation of the values contained by the fields
     * of this object. If we are in debug mode we return more information.
     * @param debug Boolean indicating if we should display debugging information
     * @return String
     */
    public String toString(boolean debug) {
        String out = "";
        if (debug) {
            for (gameCard c : this) {
                out = out.concat(c.getQuantity() + " " + c.getID() + " " + c.getDeck() +" " + c.getName() + nl);
            }
        } else {
            for (gameCard c : this) {
                out = out.concat(c.getQuantity() + " " + c.getName() + nl);
            }
        }
        if (out.isEmpty()) {
            out = "Deck Empty!";
        }
        return out;
    }

    /**
     * Returns the amount of cards in this deck. This is different from the getSize()
     * method which returns the size of the ArrayList as each entity in the list also
     * contains a quantity.
     * @return int The amount of cards contained by this deck.
     */
    public int amountOfCards() {
        int i = 0;
        for (gameCard card : this){
            i = i + card.getQuantity();
        }
        return i;
    }
    
    /**
     * When we first retrieve the game deck from Shandalar we have a deck made up
     * of lots of individual cards. This method groups all the matching ones together 
     * making it the same as the format used by Duel decks.
     * 
     * This method removes the individual deck numbers for each card in the returned deck
     * as the duel format doesn't store deck information.
     */
    public gameDeck group() {
        try {
            gameCard previousCard = null;
            gameDeck tempDeck = new gameDeck();
            // if there is only one card in the deck then deal with it here
            if (this.size() == 1) {
                tempDeck.add(this.get(0).clone());
                return tempDeck;
            }
            int i = 1;
            int x = 0;

            // group the cards
            for (gameCard c : this) {
                ++x;                         // increase our count of card instances
                if (previousCard == null) {  // if this is the first card just copy it and move on
                    previousCard = c.clone();
                    continue;
                }
                if (previousCard.getName().equals(c.getName())) { // if the last card and this card are the same
                    ++i;                                          // increase our count of this card
                } else {                                          // if they are different
                    previousCard.setQuantity(i);                  // set the quantity to the amount recorded
                    if (tempDeck.contains(previousCard)) {        // if it is already in the temp deck
                        gameCard gc = tempDeck.get(previousCard); // get the previous instance
                        int totalQuantity = gc.getQuantity() + previousCard.getQuantity();
                        gc.setQuantity(totalQuantity);            // and increase its quantity to the new value
                    } else {
                        tempDeck.add(previousCard);               // otherwise add it in
                    }
                    i = 1;                                        // and reset the quantity counter
                }
                previousCard = c.clone();                         // set the preious card to the current one
                if (x == this.size()) {                           // if we're on the last card                   
                    previousCard.setQuantity(i);                  // add it in using the same check as above
                    if (tempDeck.contains(previousCard)) {        
                        gameCard gc = tempDeck.get(previousCard); 
                        int totalQuantity = gc.getQuantity() + previousCard.getQuantity();
                        gc.setQuantity(totalQuantity);            
                    } else {
                        tempDeck.add(previousCard);
                    }
                }
            }
            return tempDeck;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst grouping cards in gameDeck class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }

    /**
     * Creates a new deck containing only the cards that are stored in the given 
     * deck number.
     * @param number The deck number of the stored cards to return.
     * @return 
     */
    public gameDeck getSubDeck(int number) {
        try{
            gameDeck tempDeck = new gameDeck();
            for (gameCard c : this){
                String deck = c.getDeck().substring(0, 2);
                int deckMask = Integer.parseInt(deck, 16);                
                if (number == 3) number = 4;
                // bit comparison for deck bitflags
                if ((deckMask & number) == number){
                    tempDeck.add(c);
                }
            }
            return tempDeck;
            } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst getting subDeck in gameDeck class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }
    
    /**
     * Converts this game deck into a duel deck as used in Duel.
     * This is done by calling the convert method for each card in the
     * deck and adding them to a duelDeck object which is then returned.
     * 
     * We lose the deck information of each card by doing this because it isn't stored
     * in the duel deck format.
     * @param cardMap The mapping of the cards' name to the cards' ids.
     * @return duelDeck
     */
    public duelDeck convert(allCards cardMap) {
        try {
            duelDeck outDeck = new duelDeck();
            for (gameCard c : this) {
                duelCard convertedCard = c.convert(cardMap);
                if (convertedCard == null) {
                    continue;
                }
                outDeck.add(convertedCard);
            }
            return outDeck;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst converting from game deck to duel deck in gameDeck class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }

    /**
     * Creates the given game deck in this game deck using the following rules:
     * Any cards that are needed and are already in the game deck are used.
     * Any cards that are needed and are not already in the game deck are added to it then
     * used.
     * Cards that make up part of another deck have their other commitments preserved.
     * Any cards that aren't needed by any deck are kept in hand.
     * @param gameDeck The game deck to create in this game deck.
     * @param deckNumber The slot to inject this deck into.
     * @return 
     */
    public gameDeck setDeck(int deckNumber, gameDeck newSubDeck) {
        if (deckNumber == 3) deckNumber = 4;
        gameDeck newGameDeck = new gameDeck();
        boolean found;                
        //we need to use iterators here as we are manipulating the lists as we travel
        for (Iterator heldIt = this.iterator(); heldIt.hasNext();){ //for each held card
            gameCard heldC = (gameCard) heldIt.next();
            found = false;                                      
            for (Iterator neededIt = newSubDeck.iterator(); neededIt.hasNext();){//and each card we need
                gameCard neededC = (gameCard) neededIt.next();
                if (heldC.getID().equals(neededC.getID())){     //if they match 
                    heldC.setDeck(deckNumber, true);            //set the cards new deck
                    newGameDeck.add(heldC);                     //add it in to new deck
                    heldIt.remove();                            //remove from held
                    neededIt.remove();                          //remove from needed
                    found = true;
                    break;
                }
            }//we've been round all the cards we need and this one isn't needed
            if (!found){
                heldC.unSetDeck(deckNumber); //remove it from the deck we are making
                newGameDeck.add(heldC);      //but add it into the hand anyway
                heldIt.remove();
            }
        }
        //come this point we've been round all the cards we have and used the ones we could
        //but we might still need more in which case we'll need to add them in
        for (gameCard neededC: newSubDeck){
            neededC.setDeck(deckNumber, true);
            newGameDeck.add(neededC);
        }
        return newGameDeck;
    }
     
    /**
     * Checks if this deck contains the given card based solely on ID.
     * @param gc The card to check if it contains.
     * @return boolean true if it contains it.
     */
    public boolean contains(gameCard gc){
        for (gameCard c: this){
            if (c.getID().equals(gc.getID())){
                return true;
            }
        }
        return false;
    }
 
    /**
     * Returns the first instance of the given card from the deck.
     * @param gc The card to return
     * @return 
     */
    public gameCard get(gameCard gc){
        for (gameCard c: this){
            if (c.getName().equals(gc.getName())){
                return c;
            }
        }
        return null;
    }
}