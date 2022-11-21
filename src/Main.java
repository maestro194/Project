import java.io.IOException;

public class Main {
  public static void main(String[] args) throws IOException {
    Nonogram nonogram = new Nonogram("Test_2_Cat.txt");

    int totalRun = 33;

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

      // range recalculaiton
      nonogram.firstLastRecalculation();

      // board debug
      nonogram.printAB();

      // board after run
//      nonogram.printBoard();
    }

    nonogram.printBoard();

    // closing
    nonogram.close();
  }
}