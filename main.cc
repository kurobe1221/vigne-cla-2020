#include <cstdio>
#include <cstdlib>

//////////////////////////////
// マクロ・定数
//////////////////////////////
#define IMAGE_HEIGHT 20
#define IMAGE_WIDTH 20
#define PARTS_HEIGHT 10
#define PARTS_WIDTH 10
#define BASE_FILE_NAME "noguchi_parts.txt"
#define TARGET_FILE_NAME "kitazato_parts.txt"

//////////////////////////////
// 型定義
//////////////////////////////
typedef struct {
  uint8_t brightness[PARTS_HEIGHT][PARTS_WIDTH];
} parts_t;

typedef struct {
  parts_t parts[IMAGE_HEIGHT][IMAGE_WIDTH];
} image_t;

#pragma pack(2)
typedef struct {
  uint16_t type;            // ファイルタイプ
  uint32_t file_size;       // ファイルサイズ
  uint16_t reserved1;       // 予約領域1
  uint16_t reserved2;       // 予約領域2
  uint32_t offset;          // ファイル先頭から画像データまでのオフセット
  uint32_t info_size;       // 情報ヘッダサイズ
  int32_t width;            // 画像の幅
  int32_t height;           // 画像の高さ
  uint16_t plane;           // プレーン数
  uint16_t bit;             // ビット数
  uint32_t compression;     // 圧縮形式
  uint32_t image_size;      // 画像領域サイズ
  int32_t ppm_x;            // 横方向解像度
  int32_t ppm_y;            // 縦方向解像度
  uint32_t color_used;      // 格納パレット数
  uint32_t color_important; // 重要色数
} bmp_header_t;
#pragma pack()

typedef struct {
  uint8_t blue;
  uint8_t green;
  uint8_t red;
  uint8_t reserved;
} bmp_color_t;

//////////////////////////////
// プロトタイプ宣言
//////////////////////////////
image_t* create_image(char const* const file_name);
int export_bmp(char const* const file_name, image_t const* const image);

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

  // 対象となる画像オブジェクトの生成
  printf("create image [%s] ... ", TARGET_FILE_NAME);
  image_t* target_image = create_image(TARGET_FILE_NAME);
  if (target_image == NULL) {
    free(base_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  printf("export bmp ... ");
  if(export_bmp("test.bmp", base_image) != 0) {
    free(base_image);
    free(target_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // メモリ開放
  free(base_image);
  free(target_image);
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
          parts->brightness[py][px] = (uint8_t)brightness;
        }
      }
    }
  }

  fclose(fp);
  return image;
}

//////////////////////////////
// BMP ファイルにエクスポート
//////////////////////////////
int export_bmp(char const* const file_name, image_t const* const image) {
  FILE* fp = fopen(file_name, "wb");
  if (fp == NULL) return -1;

  int const bmp_file_header_size = 14;
  int const bmp_info_header_size = 40;
  int const bmp_color = 256;
  int const bmp_color_byte = 4;
  int const bmp_header_size = bmp_file_header_size + bmp_info_header_size;
  int const bmp_color_size = bmp_color * bmp_color_byte;
  int const height = IMAGE_HEIGHT * PARTS_HEIGHT;
  int const width = IMAGE_WIDTH * PARTS_WIDTH;
  int const width_align = width + (width % 4);

  // BMPヘッダー書き込み
  bmp_header_t header;
  header.type = 0x4D42;
  header.file_size = bmp_header_size + bmp_color_size + height * width_align;
  header.reserved1 = 0;
  header.reserved2 = 0;
  header.offset = bmp_header_size + bmp_color_size;
  header.info_size = bmp_info_header_size;
  header.width = width;
  header.height = height;
  header.plane = 1;
  header.bit = 8;
  header.compression = 0;
  header.image_size = height * width_align;
  header.ppm_x = 0;
  header.ppm_y = 0;
  header.color_used = 0;
  header.color_important = 0;

  if (fwrite(&header, bmp_header_size, 1, fp) < 1) {
    fclose(fp);
    return -1;
  }

  // カラーパレット書き込み
  bmp_color_t color[bmp_color];
  for (int i = 0; i < bmp_color; ++ i) {
    color[i].blue = i;
    color[i].green = i;
    color[i].red = i;
    color[i].reserved = 0;
  }

  if (fwrite(color, bmp_color_size, 1, fp) < 1) {
    fclose(fp);
    return -1;
  }

  // 画像領域書き込み
  int const buffer_size = height * width_align;
  uint8_t buffer[buffer_size];
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      parts_t const* const parts = &(image->parts[iy][ix]);
      for (int py = 0; py < PARTS_HEIGHT; ++ py) {
        for (int px = 0; px < PARTS_WIDTH; ++ px) {
          int const idx = (IMAGE_HEIGHT - iy - 1) * width_align * PARTS_HEIGHT +
                          (PARTS_HEIGHT - py - 1) * width_align + ix * PARTS_WIDTH + px;
          buffer[idx] = parts->brightness[py][px];
        }
      }
    }
  }

  if (fwrite(buffer, buffer_size, 1, fp) < 1) {
    fclose(fp);
    return -1;
  }

  fclose(fp);
  return 0;
}