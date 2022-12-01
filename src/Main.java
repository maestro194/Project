import java.io.IOException;

public class Main {
  public static void main(String[] args) throws IOException {
    /* test explain
      Test 1: basic, only line solving
      Test 2: basic, only line solving but bigger
      Test 3: line solving with blanks
      Test 4: blanks and small logic thinking pattern
      Test 5: unusual solving pattern
      Test 6: large puzzle with line solving and palindrome structure
     */

    // testing config
    String testname = "Test_4_Bucks.txt";
    int MAX_RUN_COUNT = 50;

    // puzzle declaration
    Nonogram nonogram = new Nonogram(testname);
    long startTime = System.nanoTime();
    int totalRun = 0;

    // solving
    for (; ; totalRun++) {
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
      nonogram.colDPSolving();
      nonogram.rowDPSolving();

      // small heuristic help
      nonogram.fastLineSolving();

      // guessing
      nonogram.guessing();

      // debug
      nonogram.printBoard(totalRun);

      // Solution check
      if (nonogram.checkAnswer() || totalRun == MAX_RUN_COUNT) {
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