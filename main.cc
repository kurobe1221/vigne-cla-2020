#include <cstdio>
#include <cstdlib>

//////////////////////////////
// マクロ・定数
//////////////////////////////
#define IMAGE_HEIGHT 20
#define IMAGE_WIDTH 20
#define PARTS_HEIGHT 10
#define PARTS_WIDTH 10
#define ROTATION_SISE 4
#define BASE_FILE_NAME "noguchi_parts.txt"
#define TARGET_FILE_NAME "kitazato_parts.txt"

//////////////////////////////
// 型定義
//////////////////////////////
typedef struct {
  unsigned char brightness[PARTS_HEIGHT][PARTS_WIDTH];
} parts_t;

typedef struct {
  parts_t parts[IMAGE_HEIGHT][IMAGE_WIDTH];
} image_t;

//////////////////////////////
// プロトタイプ宣言
//////////////////////////////
image_t* create_image(char const* const file_name);

//////////////////////////////
// エントリーポイント
//////////////////////////////
int main() {

  // ベースとなる画像オブジェクトの生成
  printf("create image [%s] ... ", BASE_FILE_NAME);
  image_t* base_image = create_image(BASE_FILE_NAME);
  if (base_image == NULL) {
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // メモリ開放
  free(base_image);
  return 0;
}

//////////////////////////////
// 画像オブジェクトの生成
//////////////////////////////
image_t* create_image(char const* const file_name) {

  // ファイルオープン
  FILE* fp = fopen(file_name, "r");
  if (fp == NULL) return NULL;

  // メモリ確保
  image_t* image = (image_t*)malloc(sizeof(image_t));
  if (image == NULL) {
    fclose(fp);
    return NULL;
  }

  // ファイル読み込み
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      int no = 0;
      if (fscanf(fp, "%d", &no) == EOF) {
        fclose(fp);
        free(image);
        return NULL;
      }
      parts_t* const parts = &(image->parts[iy][ix]);
      for (int py = 0; py < PARTS_HEIGHT; ++ py) {
        for (int px = 0; px < PARTS_WIDTH; ++ px) {
          int brightness = 0;
          if (fscanf(fp, "%d", &brightness) == EOF) {
            fclose(fp);
            free(image);
            return NULL;
          }
          parts->brightness[py][px] = (unsigned char)brightness;
        }
      }
    }
  }

  fclose(fp);
  return image;
}