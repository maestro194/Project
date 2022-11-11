import java.io.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class Nonogram {
  public static int WIDTH;
  public static int HEIGHT;
  public static List<List<Integer>> rowClue;
  public static List<List<Integer>> colClue;
  public static int[][] board;

  public Nonogram() {
    // read the test
    Scanner scanner;
    try {
      File file = new File(".");
      scanner = new Scanner(new File(file.getAbsolutePath() + "/res/Test/Test_1_Dancer.txt"));
    } catch (FileNotFoundException e) {
      throw new RuntimeException(e);
    }

    // set up the board
    WIDTH = scanner.nextInt();
    HEIGHT = scanner.nextInt();
    board = new int[HEIGHT][WIDTH];
    rowClue = new ArrayList<>();
    colClue = new ArrayList<>();
    scanner.nextLine();

    // read the width clue
    for(int i = 0; i < WIDTH; i ++) {
      String s = scanner.nextLine();
      colClue.add(stringToList(s));
    }

    // read the height clue
    for(int i = 0; i < HEIGHT; i ++) {
      String s = scanner.nextLine();
      rowClue.add(stringToList(s));
    }
  }

  public void naive(int i, int j, int row) {
    if(row == HEIGHT) {
      if(checkAnswer()) {
        printBoard();
        System.exit(0);
      }
      return;
    }

    if(!checkValid())
      return;

    if(i >= WIDTH) {
      if(j < rowClue.get(row).size())
        return;
      naive(0, 0, row + 1);
      return;
    }

    if(j == rowClue.get(row).size()) {
      naive(WIDTH, j, row);
      return;
    }

    if(!fitRow(row, j, i))
      return;

    int len = rowClue.get(row).get(j);
    for(int k = i; k < i + len; k ++)
      board[row][k] = 1;

    printBoard();
    naive(i + len + 1, j + 1, row);

    for(int k = i; k < i + len; k ++)
      board[row][k] = 0;
    naive(i + 1, j, row);
  }

  public void lineSolving(int type, int line) {

  }

  private ArrayList<Integer> stringToList(String s) {
    ArrayList<Integer> val = new ArrayList<>();
    int x = 0;

    for(int i = 0; i < s.length(); i ++) {
      if(s.charAt(i) == ' ') {
        val.add(x);
        x = 0;
      } else {
        x = x * 10 + (s.charAt(i) - '0');
      }
    }
    val.add(x);
    return val;
  }

  public void printClue() {
    for(int i = 0; i < WIDTH; i ++) {
      List<Integer> tmp = colClue.get(i);
      for (Integer integer : tmp) {
        System.out.print(integer + " ");
      }
      System.out.println();
    }
    for(int i = 0; i < HEIGHT; i ++) {
      List<Integer> tmp = rowClue.get(i);
      for (Integer integer : tmp) {
        System.out.print(integer + " ");
      }
      System.out.println();
    }
  }

  public void printBoard() {
    for(int i = 0; i < HEIGHT; i ++) {
      for(int j = 0; j < WIDTH; j ++) {
        System.out.print(board[i][j]);
      }
      System.out.println();
    }
    System.out.println();
  }

  public boolean checkAnswer() {
    int x;
    int it;
    List<Integer> tmp;

    for(int i = 0; i < WIDTH; i ++) {
      x = 0;
      it = 0;
      tmp = colClue.get(i);
      for(int j = 0; j < HEIGHT; j ++) {
        if(board[j][i] == 0) {
          if(x != 0) {
            if(it == tmp.size()) {
              return false;
            } else {
              if (x == tmp.get(it)) {
                it++;
                x = 0;
              } else {
                return false;
              }
            }
          }
        } else {
          x ++;
        }
      }

      if(x != 0) {
        if(it == tmp.size()) {
          return false;
        } else {
          if(x != tmp.get(it))
            return false;
        }
      }

      if(it != tmp.size())
        return false;
    }

    return true;
  }

  public boolean checkValid() {
    for(int i = 0; i < WIDTH; i ++) {
      if(!checkCol(i))
        return false;
    }
    return true;
  }

  public boolean checkCol(int col) {
    List<Integer> tmp = colClue.get(col);
    boolean flag = false;
    int it = 0;

    for(int i = 0; i < HEIGHT; i ++) {
      if(board[i][col] == 1) {
        if(it == tmp.size())
          return false;

        int len = tmp.get(it ++);
        for(int j = i; j < i + len; j ++) {
          if(j == HEIGHT)
            return false;
          if(board[j][col] == 0) {
            for(int k = j + 1; k < HEIGHT; k ++)
              if(board[k][col] == 1)
                return false;
            return true;
          }
        }

        i = i + len - 1;
        if(i + 1 == HEIGHT)
          return true;
        if(board[i + 1][col] == 1)
          return false;
      }
    }

    return true;
  }

  public boolean fitRow(int row, int it, int pos) {
    int tmp = 0;
    for(int k = it; k < rowClue.get(row).size(); k ++)
      tmp += rowClue.get(row).get(k) + 1;
    if(tmp - 1 > WIDTH - pos)
      return false;
    return true;
  }
}
