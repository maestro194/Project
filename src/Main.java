import java.io.IOException;
import java.util.Scanner;

public class Main {
  public static void main(String[] args) throws IOException {
    Nonogram nonogram = new Nonogram();

      for(int j = 0; j < Nonogram.rowClue.size(); j ++) {
        System.out.println("Row " + j + ":");
        for(int k = 0; k < Nonogram.rowClue.get(j).size(); k ++) {
          System.out.print(Nonogram.rowClueLeft.get(j).get(k) + " ");
          System.out.println(Nonogram.rowClueRight.get(j).get(k));
        }
        System.out.println();
      }


//    for(int i = 0; i < Nonogram.WIDTH; i ++)
//      nonogram.colSimpleBoxes(i);
//    for(int i = 0; i < Nonogram.HEIGHT; i ++)
//      nonogram.rowSimpleBoxes(i);
//    nonogram.printBoard();
  }
}