import java.io.IOException;
import java.util.Scanner;

public class Main {
  public static void main(String[] args) throws IOException {
    Nonogram nonogram = new Nonogram();

    // simple boxes method
    for(int i = 0; i < Nonogram.WIDTH; i ++)
      nonogram.colSimpleBoxes(i);
    for(int i = 0; i < Nonogram.HEIGHT; i ++)
      nonogram.rowSimpleBoxes(i);

    // simple spaces method
    for(int i = 0; i < Nonogram.WIDTH; i ++)
      nonogram.colSimpleSpaces(i);
    nonogram.rowSimpleSpaces();

    // board after solve
    nonogram.printBoard();

    // closing
    nonogram.close();
  }
}