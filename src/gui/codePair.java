package gui;

/**
 * Used to store both the Duel and Shandalar code for a card.
 * The Duel code is stored is as an int since it is stored as
 * one in game anyway. The Game code is stored as a String 
 * since it is a 2 byte hexidecimal number.
 * @author Ryan Russell
 */
public class codePair {

    /**
     * Empty constructor creates the object but doesn't
     * initiate anything.
     */
    public codePair() {
    }

    /**
     * Creates a codePair object and sets the fields to the arguments.
     * @param duelCode An int representing the duel card code.
     * @param gameCode A String representing the game card code.
     */
    public codePair(int duelCode, String gameCode) {
        duel = duelCode;
        game = gameCode;
    }

    /**
     * Returns a string representation of the codePair.
     * @return String The values held in the fields.
     */
    @Override
    public String toString() {
        return "(" + duel + ", " + game + ")";
    }

    /**
     * Gets the duel code int in the pair
     * @return Returns the duel int
     */
    public int getduelCode() {
        return duel;
    }

    /**
     * Sets the duel code int in the pair
     * @param duelCode The new duel code int
     */
    public void setduelCode(int duelCode) {
        duel = duelCode;
    }
    private int duel = 0;

    /**
     * Gets the game code string in the pair
     * @return Returns the game string
     */
    public String getgameCode() {
        return game;
    }

    /**
     * Sets the game code string in the pair
     * @param gameCode The new game code string
     */
    public void setgameCode(String gameCode) {
        game = gameCode;
    }
    private String game = null;
}
