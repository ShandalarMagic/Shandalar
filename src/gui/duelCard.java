package gui;

import javax.swing.JOptionPane;

/**
 * The same as the gameCard but for cards from the duel deck.
 * @author Ryan Russell
 * @see gameCard
 */
public class duelCard {

    static String nl = System.getProperty("line.separator");    
    private int id, quantity;
    private String name;
    
    /**
     * Empty constructor just initiates the variables.
     */
    public duelCard(){
        id = quantity = 0;
        name = "";
    }
    
    /**
     * 3 argument constructor sets id, quantity and name
     * @param id
     * @param quantity
     * @param name 
     */
    public duelCard(int id, int quantity, String name){
    this.id=id;
    this.quantity=quantity;
    this.name = name;            
    }
    
    // <editor-fold defaultstate="collapsed" desc="Getters and setters">
    public int getID(){return this.id;}
    public void setID(int id){this.id=id;}
    public int getQuantity(){return this.quantity;}
    public void setQuantity(int quantity){this.quantity=quantity;}
    public String getName(){return this.name;}
    public void setName(String name){this.name=name;}
// </editor-fold>
    
    /**
     * This method is used to convert a duel card into a game card.
     * We do so by taking a map of how the two cards are joined and
     * then getting the codePair that matches the name of the card.
     * From this we can get the new code and the the quantity is 
     * just copied across. 
     * @param cardMap The mapping of card names' to card codes'.
     * @return gameCard A game card representation of the duel card.
     */
    public gameCard convert(allCards cardMap) {
        try {
            gameCard outCard = new gameCard();
            outCard.setName(name);
            String newID = cardMap.duelToGame(id);
            if (newID == null) {
                return null;
            }
            outCard.setID(newID);
            outCard.setQuantity(1);
            return outCard;
        } catch (Exception e) {
            JOptionPane.showMessageDialog(null, "Error whilst converting from duel card to game card in duelCard class." + nl + "e.getMessage():"
                    + e.getMessage(), null, JOptionPane.ERROR_MESSAGE);
            return null;
        }
    }

}