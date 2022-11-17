import java.io.IOException;

public class Main {
  public static void main(String[] args) throws IOException {
    Nonogram nonogram = new Nonogram("Test_2_Cat.txt");

    int totalRun = 32;

    for(int run = 0; run < totalRun; run ++) {
      // simple boxes method
      nonogram.rowSimpleBoxes();
      nonogram.colSimpleBoxes();

      // simple spaces method
      nonogram.rowSimpleSpaces();
      nonogram.colSimpleSpaces();

      // forcing method
      nonogram.rowForcing();
      nonogram.colForcing();

      nonogram.printAB();

      // board after run
//      nonogram.printBoard();
    }

    nonogram.printBoard();

    // closing
    nonogram.close();
  }
}