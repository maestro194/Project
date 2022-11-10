public class Nonogram {
  public static int WIDTH;
  public static int HEIGHT;
  public static int[][] board;

  public Nonogram(int width, int height) {
    // set up the board
    WIDTH = width;
    HEIGHT = height;
    board = new int[WIDTH][HEIGHT];

    // read the width clue
    for(int i = 0; i < width; i ++) { 
      // read clue
    }

    // read the height clue
    for(int i = 0; i < height; i ++) {
      // read clue
    }
  }

  /**
   * simplifying line with x check
   * @param type 0 for row, 1 for col
   * @param line the line that is checked
   */
  public void simplifyLine(int type, int line) {

  }

  /**
   * brute force checking if a line has white space that can be checked.
   * @param type 0 for row, 1 for col
   * @param line the line that is checked
   */
  public void bruteLine(int type, int line) {
    
  }
  

  public boolean answerChecker() {
    return true;
  }


}
