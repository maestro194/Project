import java.io.IOException;
import java.util.Scanner;

public class Main {
  public static void main(String[] args) throws IOException {
    Nonogram nonogram = new Nonogram();

//    nonogram.printClue();

    nonogram.naive(0, 0, 0);
  }
}