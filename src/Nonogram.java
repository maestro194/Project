import java.io.*;
import java.util.ArrayList;
import java.util.List;
import java.util.Scanner;

public class Nonogram {
  public static int WIDTH;
  public static int HEIGHT;
  public static List<List<Integer>> rowClue;
  public static List<List<Integer>> colClue;
  public static List<List<Integer>> rowClueLeft;
  public static List<List<Integer>> colClueLeft;
  public static List<List<Integer>> rowClueRight;
  public static List<List<Integer>> colClueRight;
  public static int[][] board;
  public static FileWriter fileWriter;
  public static BufferedWriter bufferedWriter;

  public Nonogram() {
    // read the test
    Scanner scanner = new Scanner(System.in);

    try {
      File file = new File(".");
      scanner = new Scanner(new File(file.getAbsolutePath() + "/res/Test/Test_1_Dancer.txt"));
      fileWriter = new FileWriter(file.getAbsolutePath() + "/res/Output/output.txt");
    } catch (IOException e) {
      e.printStackTrace();
    }

    // set up the board
    WIDTH = scanner.nextInt();
    HEIGHT = scanner.nextInt();
    board = new int[HEIGHT][WIDTH];       // change to bitmask
    rowClue = new ArrayList<>();
    colClue = new ArrayList<>();
    rowClueLeft = new ArrayList<>();
    colClueLeft = new ArrayList<>();
    rowClueRight = new ArrayList<>();
    colClueRight = new ArrayList<>();
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

  // set up left and right clue range
    // row
    for(int i = 0; i < HEIGHT; i ++) {
      List<Integer> tmpLeft = new ArrayList<>();
      List<Integer> tmpRight = new ArrayList<>();
      List<Integer> tmpClue = rowClue.get(i);

      for(int j = 0; j < tmpClue.size(); j ++) {
        int num;
        // left
        if(j == 0) {
          tmpLeft.add(0);
        } else {
          num = j;
          for(int k = 0; k < j; k ++)
            num += tmpClue.get(k);
          tmpLeft.add(num);
        }

        // right
        num = WIDTH - 1 - (tmpClue.get(j) - 1);
        for(int k = j + 1; k < tmpClue.size(); k ++)
          num -= tmpClue.get(k) + 1;
        tmpRight.add(num);
      }

      rowClueLeft.add(tmpLeft);
      rowClueRight.add(tmpRight);
    }

    // col
    for(int j = 0; j < WIDTH; j ++) {
      List<Integer> tmpLeft = new ArrayList<>();
      List<Integer> tmpRight = new ArrayList<>();
      List<Integer> tmpClue = colClue.get(j);

      for(int i = 0; i < tmpClue.size(); i ++) {
        int num;
        // left
        if(i == 0) {
          tmpLeft.add(0);
        } else {
          num = i;
          for(int k = 0; k < i; k ++)
            num += tmpClue.get(k);
          tmpLeft.add(num);
        }

        // right
        num = HEIGHT - 1 - (tmpClue.get(i) - 1);
        for(int k = i + 1; k < tmpClue.size(); k ++)
          num -= tmpClue.get(k) + 1;
        tmpRight.add(num);
      }

      colClueLeft.add(tmpLeft);
      colClueRight.add(tmpRight);
    }
  }

  public void naive(int i, int j, int row) throws IOException {
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

    // printBoard();

    naive(i + len + 1, j + 1, row);

    for(int k = i; k < i + len; k ++)
      board[row][k] = 0;
    naive(i + 1, j, row);
  }

  public void rowSimpleBoxes(int row) {
    List<Integer> clue = rowClue.get(row);

    for(int i = 0; i < clue.size(); i ++) {
      int front = rowClueLeft.get(row).get(i);
      int back = rowClueRight.get(row).get(i) + clue.get(i) - 1;

      // filling
      if(back - front + 1 >= clue.get(i) * 2)
        continue;
      int l = back + 1 - clue.get(i);
      int r = front - 1 + clue.get(i);

      for(int j = l; j <= r; j ++)
        board[row][j] = 1;
    }
  }

  public void colSimpleBoxes(int col) throws IOException {
    List<Integer> clue = colClue.get(col);

    for(int i = 0; i < clue.size(); i ++) {
      int front = colClueLeft.get(col).get(i);
      int back = colClueRight.get(col).get(i) + clue.get(i) - 1;

      // filling
      if(back - front + 1 >= clue.get(i) * 2)
        continue;
      int l = back + 1 - clue.get(i);
      int r = front - 1 + clue.get(i);

      if(l > r)
        continue;

      for(int j = l; j <= r; j ++)
        board[j][col] = 1;
    }
  }

  public void rowSimpleSpaces() {
    for(int u = 0; u < HEIGHT; u ++) { // each row
      for (int i = 0; i < WIDTH; i ++) { // comp of row
        for (int v = 0; v < rowClue.get(u).size(); v++) { // clue of row

        }
      }
    }
  }

  public void colSimpleSpaces(int col) {
    for(int i = 0; i < HEIGHT; i ++) {
      boolean flagUp = true;
      boolean flagDown = true;

      for(int j = 0; j < colClue.get(col).size(); j ++) {
        if(colClueLeft.get(col).get(j) <= i)
          flagUp = false;
        if(colClueRight.get(col).get(j) + colClue.get(col).get(j) >= i)
          flagDown = false;
      }

      if(flagUp || flagDown)
        board[i][col] = 2;
    }
  }

  public ArrayList<Integer> stringToList(String s) {
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

  public void printBoard() throws IOException {
    for(int i = 0; i < HEIGHT; i ++) {
      for(int j = 0; j < WIDTH; j ++) {
        fileWriter.write((board[i][j] == 1 ? '*' : (board[i][j] == 2 ? 'x' : '.')));
      }
      fileWriter.write('\n');
    }
    fileWriter.write('\n');
    fileWriter.flush();
  }

  public void printLR() {
    for(int j = 0; j < Nonogram.rowClue.size(); j ++) {
      System.out.println("Row " + j + ":");
      for(int k = 0; k < Nonogram.rowClue.get(j).size(); k ++) {
        System.out.print(Nonogram.rowClueLeft.get(j).get(k) + " ");
        System.out.println(Nonogram.rowClueRight.get(j).get(k));
      }
      System.out.println();
    }

    for(int i = 0; i < Nonogram.colClue.size(); i ++) {
      System.out.println("Col " + i + ":");
      for(int j = 0; j < Nonogram.colClue.get(i).size(); j ++) {
        System.out.print(Nonogram.colClueLeft.get(i).get(j) + " ");
        System.out.println(Nonogram.colClueRight.get(i).get(j));
      }
      System.out.println();
    }
  }

  public void close() throws IOException {
    if(fileWriter != null)
      fileWriter.close();
  }

  public boolean checkAnswer() {
    int it;
    List<Integer> tmp;

    for(int j = 0; j < WIDTH; j ++) {
      it = 0;
      tmp = colClue.get(j);
      for(int i = 0; i < HEIGHT; i ++) {
        if(board[i][j] == 1) {
          if(it == tmp.size())
            return false;

          int len = tmp.get(it ++);
          for(int k = i; k < i + len; k ++) {
            if (k == HEIGHT)
              return false;
            if (board[k][j] != 1)
              return false;
          }

          i += len;
          if(i == HEIGHT)
            break;
          if(board[i][j] == 1)
            return false;
        }
      }

      if(it < tmp.size())
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
