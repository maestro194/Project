import java.io.*;
import java.util.ArrayList;
import java.util.Arrays;
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
  public static int[][][] alpha;
  public static int[][][] beta;
  public static FileWriter fileWriter;

  public Nonogram(String test) {
    // read the test
    Scanner scanner = new Scanner(System.in);

    try {
      File file = new File(".");
      scanner = new Scanner(new File(file.getAbsolutePath() + "/res/Test/" + test));
      fileWriter = new FileWriter(file.getAbsolutePath() + "/res/Output/output.txt");
    } catch (IOException e) {
      e.printStackTrace();
    }

    // set up the board
    WIDTH = scanner.nextInt();
    HEIGHT = scanner.nextInt();
    board = new int[HEIGHT][WIDTH];       // change to bitmask
    alpha = new int[HEIGHT][WIDTH / 2][WIDTH];
    for (int[][] t1 : alpha) for (int[] t2 : t1) Arrays.fill(t2, 0);
    beta = new int[HEIGHT / 2][WIDTH][HEIGHT];
    for (int[][] t1 : beta) for (int[] t2 : t1) Arrays.fill(t2, 0);
    rowClue = new ArrayList<>();
    colClue = new ArrayList<>();
    rowClueLeft = new ArrayList<>();
    colClueLeft = new ArrayList<>();
    rowClueRight = new ArrayList<>();
    colClueRight = new ArrayList<>();
    scanner.nextLine();

    // read the width clue
    for (int i = 0; i < WIDTH; i++) {
      String s = scanner.nextLine();
      colClue.add(stringToList(s));
    }

    // read the height clue
    for (int i = 0; i < HEIGHT; i++) {
      String s = scanner.nextLine();
      rowClue.add(stringToList(s));
    }

    // set up left and right clue range
    // row
    for (int i = 0; i < HEIGHT; i++) {
      List<Integer> tmpLeft = new ArrayList<>();
      List<Integer> tmpRight = new ArrayList<>();
      List<Integer> tmpClue = rowClue.get(i);

      for (int j = 0; j < tmpClue.size(); j++) {
        int num;
        int l, r;
        // left
        if (j == 0) {
          tmpLeft.add(0);
          l = 0;
        } else {
          num = j;
          for (int k = 0; k < j; k++)
            num += tmpClue.get(k);
          l = num;
          tmpLeft.add(num);
        }

        // right
        num = WIDTH - 1 - (tmpClue.get(j) - 1);
        for (int k = j + 1; k < tmpClue.size(); k++)
          num -= tmpClue.get(k) + 1;
        r = num;
        tmpRight.add(num);

        // alpha
        for (int k = l; k < r + tmpClue.get(j); k++)
          alpha[i][j][k] = 1;
      }

      rowClueLeft.add(tmpLeft);
      rowClueRight.add(tmpRight);
    }

    // col
    for (int j = 0; j < WIDTH; j++) {
      List<Integer> tmpLeft = new ArrayList<>();
      List<Integer> tmpRight = new ArrayList<>();
      List<Integer> tmpClue = colClue.get(j);

      for (int i = 0; i < tmpClue.size(); i++) {
        int num;
        int l, r;

        // left
        if (i == 0) {
          tmpLeft.add(0);
          l = 0;
        } else {
          num = i;
          for (int k = 0; k < i; k++)
            num += tmpClue.get(k);
          l = num;
          tmpLeft.add(num);
        }

        // right
        num = HEIGHT - 1 - (tmpClue.get(i) - 1);
        for (int k = i + 1; k < tmpClue.size(); k++)
          num -= tmpClue.get(k) + 1;
        r = num;
        tmpRight.add(num);

        for (int k = l; k < r + tmpClue.get(i); k++)
          beta[i][j][k] = 1;
      }

      colClueLeft.add(tmpLeft);
      colClueRight.add(tmpRight);
    }
  }

  public void naive(int i, int j, int row) throws IOException {
    if (row == HEIGHT) {
      if (checkAnswer()) {
        printBoard();
        System.exit(0);
      }
      return;
    }

    if (!checkValid())
      return;

    if (i >= WIDTH) {
      if (j < rowClue.get(row).size())
        return;
      naive(0, 0, row + 1);
      return;
    }

    if (j == rowClue.get(row).size()) {
      naive(WIDTH, j, row);
      return;
    }

    if (!fitRow(row, j, i))
      return;

    int len = rowClue.get(row).get(j);
    for (int k = i; k < i + len; k++)
      board[row][k] = 1;

    // printBoard();

    naive(i + len + 1, j + 1, row);

    for (int k = i; k < i + len; k++)
      board[row][k] = 0;
    naive(i + 1, j, row);
  }

  public void rowSimpleBoxes() {
    for (int row = 0; row < HEIGHT; row++) {
      List<Integer> clue = rowClue.get(row);

      for (int i = 0; i < clue.size(); i++) {
        int first = rowClueLeft.get(row).get(i);
        int last = rowClueRight.get(row).get(i) + clue.get(i) - 1;

        // filling
        if (first + clue.get(i) - 1 < last - clue.get(i) + 1)
          continue;

        int l = Math.min(Math.max(last - clue.get(i) + 1, 0), WIDTH - 1);
        int r = Math.min(Math.max(first + clue.get(i) - 1, 0), WIDTH - 1);

        for (int j = l; j <= r; j++)
          board[row][j] = 1;
      }
    }
  }

  public void colSimpleBoxes() {
    for (int col = 0; col < WIDTH; col++) {
      List<Integer> clue = colClue.get(col);

      for (int i = 0; i < clue.size(); i++) {
        int first = colClueLeft.get(col).get(i);
        int last = colClueRight.get(col).get(i) + clue.get(i) - 1;

        // filling
        if (first + clue.get(i) - 1 < last - clue.get(i) + 1)
          continue;

        int l = Math.min(Math.max(last - clue.get(i) + 1, 0), HEIGHT - 1);
        int r = Math.min(Math.max(first + clue.get(i) - 1, 0), HEIGHT - 1);

        for (int j = l; j <= r; j++)
          board[j][col] = 1;
      }
    }
  }

  public void rowSimpleSpaces() {
    for (int u = 0; u < HEIGHT; u++) { // each row
      for (int i = 0; i < WIDTH; i++) { // comp of row
        int flag = 0;
        int mem = -1;

        for (int v = 0; v < rowClue.get(u).size(); v++) { // clue of row
          if (alpha[u][v][i] == 1) {
            flag++;
            mem = v;
          }
        }

        if (flag == 0) { // for every clue V, nothing needs board(u, i)
          board[u][i] = 2;
          for (int j = 0; j < colClue.get(i).size(); j++)
            beta[j][i][u] = 0;
        }

        if (flag == 1 && board[u][i] == 1) { // if there's only 1 clue need board(u,i)
          rowClueLeft.get(u).set(mem, Math.max(i - rowClue.get(u).get(mem) + 1, rowClueLeft.get(u).get(mem)));
          rowClueRight.get(u).set(mem, Math.min(i, rowClueRight.get(u).get(mem)));
        }
      }

      for (int v = 0; v < rowClue.get(u).size(); v++) {
        for(int j = 0; j < rowClueLeft.get(u).get(v); j ++)
          alpha[u][v][j] = 0;
        for(int j = rowClueRight.get(u).get(v) + rowClue.get(u).get(v); j < WIDTH; j ++)
          alpha[u][v][j] = 0;
      }
    }
  }

  public void colSimpleSpaces() {
    for (int n = 0; n < WIDTH; n++) { // each col
      for (int i = 0; i < HEIGHT; i++) { // comp of col
        int flag = 0;
        int mem = -1;

        for (int m = 0; m < colClue.get(n).size(); m ++) { // clue of col
          if (beta[m][n][i] == 1) {
            flag ++;
            mem = m;
          }
        }

        if(flag == 0) { // for every clue m, nothing needs board(j, n)
          board[i][n] = 2;
          for (int j = 0; j < rowClue.get(j).size(); j ++)
            alpha[i][j][n] = 0;
        }

        if(flag == 1 && board[i][n] == 1) { // if there's only one clue need board(i, n)
          colClueLeft.get(n).set(mem, Math.max(i - colClue.get(n).get(mem) + 1, colClueLeft.get(n).get(mem)));
          colClueRight.get(n).set(mem, Math.min(i, colClueRight.get(n).get(mem)));
        }
      }

      for(int m = 0; m < colClue.get(n).size(); m ++) {
        for(int j = 0; j < colClueLeft.get(n).get(m); j ++)
          beta[m][n][j] = 0;
        for(int j = colClueRight.get(n).get(m) + colClue.get(n).get(m); j < HEIGHT; j ++)
          beta[m][n][j] = 0;
      }
    }
  }

  public void rowForcing() {
    for(int u = 0; u < HEIGHT; u ++) {
      for(int v = 0; v < rowClue.get(u).size(); v ++) {
        // method 4
        int first = rowClueLeft.get(u).get(v);
        int last = rowClueRight.get(u).get(v) + rowClue.get(u).get(v) - 1;
        int cnt = 0;

        for(int i = 0; i < WIDTH; i ++) {
          if(alpha[u][v][i] == 0) {
            // method 1
            if(i < rowClue.get(u).get(v))
              for(int j = 0; j < i; j ++)
                alpha[u][v][j] = 0;

            // method 2
            for(int j = 0; j < i; j ++)
              if(alpha[u][v][j] == 0 && (i - j) <= rowClue.get(u).get(v)) {
                for (int k = j + 1; k < i; k++)
                  alpha[u][v][k] = 0;
                break;
              }

            // method 3
            if(i + rowClue.get(u).get(v) >= WIDTH) {
              for(int j = i + 1; j < WIDTH; j ++)
                alpha[u][v][j] = 0;
            }
          }

          // method 4 calculation
          cnt += alpha[u][v][i];
        }

        // method 4 finalize
        if(cnt == rowClue.get(u).get(v)) {
          for(int w = 0; w < v; w ++)
            for(int i = first - 1; i < WIDTH; i ++)
              alpha[u][w][i] = 0;
          for(int w = v + 1; w < rowClue.get(u).size(); w ++)
            for(int i = 0; i <= Math.min(WIDTH, last + 1); i ++)
              alpha[u][w][i] = 0;
        }

        // left right recalculation
        while(first < last && alpha[u][v][first] == 0)
          first ++;
        while(last > first + rowClue.get(u).get(v) - 1 && alpha[u][v][last] == 0)
          last --;
        rowClueLeft.get(u).set(v, first);
        rowClueRight.get(u).set(v, last - rowClue.get(u).get(v) + 1);
      }
    }
  }

  public void colForcing() {
    for(int n = 0; n < WIDTH; n ++) {
      for(int m = 0; m < colClue.get(n).size(); m ++) {
        // method 4
        int first = colClueLeft.get(n).get(m);
        int last = colClueRight.get(n).get(m) + colClue.get(n).get(m) - 1;
        int cnt = 0;

        for(int i = 0; i < HEIGHT; i ++) {
          if(beta[m][n][i] == 0) {
            // method 1
            if(i < colClue.get(n).get(m))
              for(int j = 0; j < i; j ++)
                beta[m][n][j] = 0;

            // method 2
            for(int j = 0; j < i; j ++)
              if(beta[m][n][j] == 0 && (i - j) <= colClue.get(n).get(m)) {
                for(int k = j + 1; k < i; k ++)
                  beta[m][n][k] = 0;
                break;
              }

            // method 3
            if(i + colClue.get(n).get(m) >= HEIGHT)
              for(int j = i + 1; j < HEIGHT; j ++)
                beta[m][n][j] = 0;
          }

          // method 4 calculation
          cnt += beta[m][n][i];
        }

        // method 4 finalize
        if(cnt == colClue.get(n).get(m)) {
          for(int w = 0; w < m; w ++)
            for(int i = first - 1; i < WIDTH; i ++)
              beta[w][n][i] = 0;
          for(int w = m + 1; w < colClue.get(n).size(); w ++)
            for(int i = 0; i < Math.min(WIDTH, last + 1); i ++)
              beta[w][n][i] = 0;
        }

        // left right recalculation
        while(first < HEIGHT && beta[m][n][first] == 0)
          first ++;
        while(last > first + colClue.get(n).get(m) - 1 && beta[m][n][last] == 0)
          last --;
        colClueLeft.get(n).set(m, first);
        colClueRight.get(n).set(m, last - colClue.get(n).get(m) + 1);
      }
    }
  }

  public ArrayList<Integer> stringToList(String s) {
    ArrayList<Integer> val = new ArrayList<>();
    int x = 0;

    for (int i = 0; i < s.length(); i++) {
      if (s.charAt(i) == ' ') {
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
    for (int i = 0; i < HEIGHT; i++) {
      for (int j = 0; j < WIDTH; j++) {
        fileWriter.write((board[i][j] == 1 ? '*' : (board[i][j] == 2 ? 'x' : '.')));
      }
      fileWriter.write('\n');
    }
    fileWriter.write('\n');

    fileWriter.flush();
  }

  public void printAB() throws IOException {
    fileWriter.write("alpha board:\n");
    for (int u = 0; u < HEIGHT; u++) {
      for (int v = 0; v < rowClue.get(u).size(); v++) {
        for (int i = 0; i < WIDTH; i++) {
          int tmp = alpha[u][v][i];
          fileWriter.write((tmp == 1 ? '1' : '0'));
        }
        fileWriter.write('\n');
        for (int j = 0; j < rowClue.get(u).size(); j ++)
          fileWriter.write(rowClueLeft.get(u).get(j) + " " +
                              rowClueRight.get(u).get(j) + '\n');
      }
      fileWriter.write('\n');
    }

    fileWriter.write("beta board:\n");
    for(int n = 0; n < WIDTH; n ++) {
      for(int m = 0; m < colClue.get(n).size(); m ++) {
        for(int i = 0; i < HEIGHT; i ++) {
          int tmp = beta[m][n][i];
          fileWriter.write((tmp == 1 ? '1' : '0'));
        }
        fileWriter.write('\n');
        for(int j = 0; j < colClue.get(n).size(); j ++)
          fileWriter.write(colClueLeft.get(n).get(j) + " " +
                              colClueRight.get(n).get(j) + '\n');
      }
      fileWriter.write('\n');
    }

    fileWriter.flush();
  }

  public void close() throws IOException {
    if (fileWriter != null)
      fileWriter.close();
  }

  public boolean checkAnswer() {
    int it;
    List<Integer> tmp;

    for (int j = 0; j < WIDTH; j++) {
      it = 0;
      tmp = colClue.get(j);
      for (int i = 0; i < HEIGHT; i++) {
        if (board[i][j] == 1) {
          if (it == tmp.size())
            return false;

          int len = tmp.get(it++);
          for (int k = i; k < i + len; k++) {
            if (k == HEIGHT)
              return false;
            if (board[k][j] != 1)
              return false;
          }

          i += len;
          if (i == HEIGHT)
            break;
          if (board[i][j] == 1)
            return false;
        }
      }

      if (it < tmp.size())
        return false;
    }

    return true;
  }

  public boolean checkValid() {
    for (int i = 0; i < WIDTH; i++) {
      if (!checkCol(i))
        return false;
    }
    return true;
  }

  public boolean checkCol(int col) {
    List<Integer> tmp = colClue.get(col);
    boolean flag = false;
    int it = 0;

    for (int i = 0; i < HEIGHT; i++) {
      if (board[i][col] == 1) {
        if (it == tmp.size())
          return false;

        int len = tmp.get(it++);
        for (int j = i; j < i + len; j++) {
          if (j == HEIGHT)
            return false;
          if (board[j][col] == 0) {
            for (int k = j + 1; k < HEIGHT; k++)
              if (board[k][col] == 1)
                return false;
            return true;
          }
        }

        i = i + len - 1;
        if (i + 1 == HEIGHT)
          return true;
        if (board[i + 1][col] == 1)
          return false;
      }
    }

    return true;
  }

  public boolean fitRow(int row, int it, int pos) {
    int tmp = 0;
    for (int k = it; k < rowClue.get(row).size(); k++)
      tmp += rowClue.get(row).get(k) + 1;
    if (tmp - 1 > WIDTH - pos)
      return false;
    return true;
  }
}
