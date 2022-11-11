import java.util.Scanner;

public class Main {
  public static void main(String[] args) {
    Nonogram nonogram = new Nonogram();

//    nonogram.printClue();

    nonogram.naive(0, 0, 0);
  }
}