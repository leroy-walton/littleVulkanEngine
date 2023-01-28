#pragma once
#include "lve_game_object.hpp"
//#include "lve_model.hpp";

namespace jam {
class CASystem {
 public:
  CASystem(int rows, int cols) : m_rows(rows), m_cols(cols) {
    grid.resize(m_rows, std::vector<int>(m_cols));
    initialiseValues();
  };

  void step() {
    std::vector<std::vector<int>> newGrid(m_rows);
    for (int i = 0; i < m_rows; i++) {
      newGrid[i].resize(m_cols);
    }

    for (int i = 0; i < m_rows; i++) {
      for (int j = 0; j < m_cols; j++) {
        int neighbors = countNeighbors(i, j);
        if (grid[i][j] == 0 && neighbors == 3) {
          newGrid[i][j] = 1;
        } else if (grid[i][j] == 1 && (neighbors < 2 || neighbors > 3)) {
          newGrid[i][j] = 0;
        }
      }
    }
    grid = std::move(newGrid);
  };

  void initialiseValues() {
    for (int i = 0; i < m_rows; i++) {
            for (int j = 0; j < m_cols; j++) {
                grid[i][j] = std::rand() % 2;
            }
        }
  };

  std::vector<std::vector<int>> grid;

 private:
  
  int m_rows;
  int m_cols;

  void loadGameObjects(lve::LveGameObject::Map& gameObjects) {
    
  };
  
  int countNeighbors(int i, int j) {
    int count = 0;
    for (int x = -1; x <= 1; x++) {
      for (int y = -1; y <= 1; y++) {
        if (x == 0 && y == 0) {
          continue;
        }
        int row = (i + x + m_rows) % m_rows;
        int col = (j + y + m_cols) % m_cols;
        count += grid[row][col];
      }
    }
    return count;
  };

  
  
};

unsigned int hash(int a, int b) {
    return (a << 16) | b;
}


float rand() {
  std::srand(std::time(0));
  return (float)std::rand() / RAND_MAX;
}

}  // namespace jam