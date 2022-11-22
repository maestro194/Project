import java.io.IOException;

public class Main {
  public static void main(String[] args) throws IOException {
    String testname = "Test_1_Dancer.txt";
    Nonogram nonogram = new Nonogram(testname);
    long startTime = System.nanoTime();

    int totalRun = 0;

    for(; ; totalRun ++) {
      /*
      brute force solution

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
      */

      // DP Solution
      nonogram.rowDPSolving();
      nonogram.colDPSolving();

      if(nonogram.checkAnswer() || totalRun == 100) {
        nonogram.printBoard(totalRun);
        break;
      }
    }

    // closing
    long endTime = System.nanoTime();
    nonogram.writeRunTime(endTime - startTime);
    nonogram.close();
  }
}