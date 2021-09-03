package gui;

import java.util.ArrayList;
import java.util.Random;
import javax.swing.JOptionPane;

/**
 * A store for duel cards. Over-rides the ArrayList's toString method
 * to make one that returns the name, quantity and id of the items held.
 * @author Ryan Russell
 */
public class duelDeck extends ArrayList<duelCard> {

    static String nl = System.getProperty("line.separator");

    /**
     * A String representation of the values contained by the fields
     * of this object.
     * @return String
     */
    public String toString(boolean debug) {
        String out = "";
        if (debug) {
            for (duelCard c : this) {
                out = out.concat(c.getQuantity() + " " + c.getID() + " " + c.getName() + nl);
            }
        } else {
            for (duelCard c : this) {
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
        for (duelCard card : this) {
            i = i + card.getQuantity();
        }
        return i;
    }

    /**
     * Converts this duel deck into a game deck as used in Shandalar.
     * This is done by calling the convert method for each card in the
     * deck and adding them to a gameDeck object which is then returned.
     * @param cardMap The mapping of the cards' name to the cards' ids.
     * @return gameDeck
     */
    public gameDeck convert(allCards cardMap) {
        String lastMissingCard = "";
        try {
            gameDeck outDeck = new gameDeck();
            for (duelCard c : this) {
                gameCard convertedCard = c.convert(cardMap);
                if (convertedCard == null) {
                    if (c.getName().equals(lastMissingCard)){
                        continue;
                    }else{
                    JOptionPane.showMessageDialog(null, c.getName() + " isn't in Shandalar so can't be injected." + nl + randomMsg(), null, JOptionPane.ERROR_MESSAGE);
                    lastMissingCard = c.getName();
                    continue;
                    }
                }
                for (int i = 0; i < c.getQuantity(); ++i){
                outDeck.add(convertedCard.clone());
                }
            }
            return outDeck;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst converting from duel deck to game deck in duelDeck class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }

    /**
     * Returns a string detailing the deck in the format used in .dck saves. The rest
     * of the information (semicolons and text) has to be entered from else where.
     * @return
     */
    public String extractionString() {
        try {
            String out = "";
            for (duelCard c : this) {
                out = out.concat("." + c.getID() + "\t" + c.getQuantity() + "\t" + c.getName() + nl);
            }
            return out;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst getting extraction string in duelDeck class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }

    /**
     * Checks if this deck contains the given card, based solely on name.
     * @param dc The card to check if it contains.
     * @return boolean true if it contains it.
     */
    public boolean contains(duelCard dc){
        for (duelCard c: this){
            if (c.getName().equals(dc.getName())){
                return true;
            }
        }
        return false;
    }

    private String randomMsg() {
        Random ran = new Random();
        switch (ran.nextInt(10)){
            case 0:
                return "Shame, I liked that one as well.";
            case 1:
                return "Oh no, that was my favourite!";
            case 2:
                return "(Actually I dropped it down the back of the sofa.)";
            case 3:
                return "A player of your calibre doesn't need it anyway. ";
            case 4:
                return "Honest.";
            case 5:
                return "I looked really hard for it but it is just not there.";
            case 6:
                return "The deck is better without it anyway.";
            case 7:
                return "Besides, no-one uses that card.";
            case 8:
                return "I think the wizards stole it.";
            case 9:
                return "I recommend Plauge Rats instead.";
            default : return "You shouldn't see this line";
        }

    }
}