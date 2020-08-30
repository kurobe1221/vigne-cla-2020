#include <cstdio>
#include <cstdlib>
#include <cmath>

//////////////////////////////
// マクロ・定数
//////////////////////////////
#define IMAGE_HEIGHT 20
#define IMAGE_WIDTH 20
#define PARTS_HEIGHT 10
#define PARTS_WIDTH 10
#define ROTATION_SIZE 4
#define BASE_FILE_NAME "noguchi_parts.txt"
#define TARGET_FILE_NAME "kitazato_parts.txt"
#define RESULT_TXT "result.txt"
#define RESULT_BMP "result.bmp"

//////////////////////////////
// 型定義
//////////////////////////////
typedef struct {
  bool locked;
  int no;
  uint8_t brightness[ROTATION_SIZE][PARTS_HEIGHT][PARTS_WIDTH];
  double normalizing[ROTATION_SIZE][PARTS_HEIGHT][PARTS_WIDTH];
} parts_t;

typedef struct {
  parts_t parts[IMAGE_HEIGHT][IMAGE_WIDTH];
} image_t;

typedef struct {
  int rotation;
  parts_t const* parts;
} position_t;

typedef struct {
  position_t position[IMAGE_HEIGHT][IMAGE_WIDTH];
} mosaic_t;

typedef struct {
  int x;
  int y;
} coord_t;

typedef struct {
  coord_t coord[IMAGE_HEIGHT * IMAGE_WIDTH];
} order_t;

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
double inner_product(parts_t const* const pa, position_t const* const pb);
double inner_product(position_t const* const pa, position_t const* const pb);
order_t* create_order_by_asc();
order_t* create_order_by_desc();
void sort_mosaic(order_t const* const order, image_t* const base_image, image_t* const target_image, mosaic_t* const mosaic);
bool check_image(image_t const* const image);
bool check_mosaic(mosaic_t const* const mosaic);
image_t* create_image_by_txt(char const* const file_name);
image_t* create_image_by_bmp(char const* const file_name);
mosaic_t* create_mosaic_by_image(image_t const* const image);
mosaic_t* create_mosaic_by_txt(char const* const file_name, image_t const* const image);
int export_mosaic_to_txt(char const* const file_name, mosaic_t const* const mosaic);
int export_mosaic_to_bmp(char const* const file_name, mosaic_t const* const mosaic);
int export_image_to_txt(char const* const file_name, image_t const* const image);
int export_image_to_bmp(char const* const file_name, image_t const* const image);

//////////////////////////////
// エントリーポイント
//////////////////////////////
int main() {
  // ベースとなる画像オブジェクトの生成
  printf("create image [%s] ... ", BASE_FILE_NAME);
  image_t* base_image = create_image_by_txt(BASE_FILE_NAME);
  if (base_image == NULL) {
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // 対象となる画像オブジェクトの生成
  printf("create image [%s] ... ", TARGET_FILE_NAME);
  image_t* target_image = create_image_by_txt(TARGET_FILE_NAME);
  if (target_image == NULL) {
    free(base_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");
  
  // モザイクオブジェクトの生成
  printf("create mosaic ... ");
  mosaic_t* mosaic = create_mosaic_by_image(base_image);
  if (mosaic == NULL) {
    free(target_image);
    free(base_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // 探索順リストの生成
  printf("create order ... ");
  // order_t* order = create_order_by_asc();
  order_t* order = create_order_by_desc();
  if (order == NULL) {
    free(mosaic);
    free(target_image);
    free(base_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // モザイクの並び替え
  printf("sort mosaic ... ");
  sort_mosaic(order, base_image, target_image, mosaic);
  printf("ok\n");

  // 画像オブジェクトが全て使用されたかチェック
  printf("check image ... ");
  if (!check_image(base_image) || !check_image(target_image)) {
    free(order);
    free(mosaic);
    free(target_image);
    free(base_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // モザイクに全てのパーツが使用されているかチェック
  printf("check mosaic ... ");
  if (!check_mosaic(mosaic)) {
    free(order);
    free(mosaic);
    free(target_image);
    free(base_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // TXTにエクスポート
  printf("export txt [%s] ... ", RESULT_TXT);
  if(export_mosaic_to_txt(RESULT_TXT, mosaic) < 0) {
    free(order);
    free(mosaic);
    free(target_image);
    free(base_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // BMPにエクスポート
  printf("export bmp [%s] ... ", RESULT_BMP);
  if(export_mosaic_to_bmp(RESULT_BMP, mosaic) < 0) {
    free(order);
    free(mosaic);
    free(target_image);
    free(base_image);
    printf("error\n");
    return -1;
  }
  printf("ok\n");

  // メモリ開放
  free(order);
  free(mosaic);
  free(target_image);
  free(base_image);
  return 0;
}

//////////////////////////////
// 2 つのパーツの内積を求める
//////////////////////////////
double inner_product(parts_t const* const pa, position_t const* const pb) {
  position_t position;
  position.rotation = 0;
  position.parts = pa;
  return inner_product(&position, pb);
}

//////////////////////////////
// 2 つのパーツの内積を求める
//////////////////////////////
double inner_product(position_t const* const pa, position_t const* const pb) {
  double sum = 0.0;
  for (int py = 0; py < PARTS_HEIGHT; ++ py) {
    for (int px = 0; px < PARTS_WIDTH; ++ px) {
      sum += pa->parts->normalizing[pa->rotation][py][px] * pb->parts->normalizing[pb->rotation][py][px];
    }
  }
  return sum;
}

//////////////////////////////
// 探索順リストの生成(昇順)
//////////////////////////////
order_t* create_order_by_asc() {
  // メモリ確保
  order_t* order = (order_t*)malloc(sizeof(order_t));
  if (order == NULL) {
    return NULL;
  }

  int idx = 0;
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      coord_t coord;
      coord.x = ix;
      coord.y = iy;
      order->coord[idx ++] = coord;
    }
  }

  return order;
}

//////////////////////////////
// 探索順リストの生成(降順)
//////////////////////////////
order_t* create_order_by_desc() {
  // メモリ確保
  order_t* order = (order_t*)malloc(sizeof(order_t));
  if (order == NULL) {
    return NULL;
  }

  int idx = IMAGE_HEIGHT * IMAGE_WIDTH - 1;
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      coord_t coord;
      coord.x = ix;
      coord.y = iy;
      order->coord[idx --] = coord;
    }
  }

  return order;
}

//////////////////////////////
// モザイクの並び替え
//////////////////////////////
void sort_mosaic(order_t const* const order, image_t* const base_image, image_t* const target_image, mosaic_t* const mosaic) {
  // 与えられた順番にパーツを探索
  for (int i = 0; i < IMAGE_HEIGHT * IMAGE_WIDTH; ++ i) {
    coord_t const coord = order->coord[i];
    parts_t* const target_parts = &target_image->parts[coord.y][coord.x];
    // 重複していたら終了
    if (target_parts->locked) {
      printf("parts[%d][%d] is locked.\n", coord.y, coord.x);
      return;
    }
    // 最も内積が大きいパーツを探索
    double best_value = -1.0;
    int best_rotation = 0;
    parts_t* best_parts = NULL;
    for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
      for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
        parts_t* const base_parts = &base_image->parts[iy][ix];
        if (base_parts->locked) continue;
        position_t position;
        position.parts = base_parts;
        for (int r = 0; r < ROTATION_SIZE; ++ r) {
          position.rotation = r;
          double const value = inner_product(target_parts, &position);
          if (value > best_value) {
            best_value = value;
            best_rotation = r;
            best_parts = base_parts;
          }
        }
      }
    }
    // パーツを確定する
    position_t position;
    position.rotation = best_rotation;
    position.parts = best_parts;
    mosaic->position[coord.y][coord.x] = position;
    best_parts->locked = true;
    target_parts->locked = true;
  }
}

//////////////////////////////
// 画像オブジェクトが全て使用されたかチェック
//////////////////////////////
bool check_image(image_t const* const image) {
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      if (!image->parts[iy][ix].locked) {
        return false;
      }
    }
  }
  return true;
}

//////////////////////////////
// モザイクに全てのパーツが使用されているかチェック
//////////////////////////////
bool check_mosaic(mosaic_t const* const mosaic) {
  bool exist[IMAGE_HEIGHT * IMAGE_WIDTH];
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      exist[mosaic->position[iy][ix].parts->no - 1] = true;
    }
  }
  for (int i = 0; i < IMAGE_HEIGHT * IMAGE_WIDTH; ++ i) {
    if (!exist[i]) {
      return false;
    }
  }
  return true;
}

//////////////////////////////
// TXTから画像オブジェクトの生成
//////////////////////////////
image_t* create_image_by_txt(char const* const file_name) {
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
      parts_t* const parts = &(image->parts[iy][ix]);
      parts->locked = false;
      if (fscanf(fp, "%d", &(parts->no)) == EOF) {
        fclose(fp);
        free(image);
        return NULL;
      }
      uint64_t sum = 0;
      for (int py = 0; py < PARTS_HEIGHT; ++ py) {
        for (int px = 0; px < PARTS_WIDTH; ++ px) {
          int brightness = 0;
          if (fscanf(fp, "%d", &brightness) == EOF) {
            fclose(fp);
            free(image);
            return NULL;
          }
          parts->brightness[0][py][px] = (uint8_t)brightness;
          sum += brightness * brightness;
        }
      }
      // 90度回転した画像情報を生成
      for (int r = 1; r < ROTATION_SIZE; ++ r) {
        for (int py = 0; py < PARTS_HEIGHT; ++ py) {
          for (int px = 0; px < PARTS_WIDTH; ++ px) {
            parts->brightness[r][PARTS_HEIGHT - px - 1][py] = parts->brightness[r - 1][py][px];
          }
        }
      }
      // ベクトルの長さを求める
      double const dist = sqrt(sum);
      // 輝度の正規化
      for (int r = 0; r < ROTATION_SIZE; ++ r) {
        for (int py = 0; py < PARTS_HEIGHT; ++ py) {
          for (int px = 0; px < PARTS_WIDTH; ++ px) {
            parts->normalizing[r][py][px] = parts->brightness[r][py][px] / dist;
          }
        }
      }
    }
  }

  fclose(fp);
  return image;
}

//////////////////////////////
// BMPから画像オブジェクトの生成
//////////////////////////////
image_t* create_image_by_bmp(char const* const file_name) {
  FILE* fp = fopen(file_name, "rb");
  if (fp == NULL) return NULL;

  int const bmp_file_header_size = 14;
  int const bmp_info_header_size = 40;
  int const bmp_color = 256;
  int const bmp_color_byte = 4;
  int const bmp_header_size = bmp_file_header_size + bmp_info_header_size;
  int const bmp_color_size = bmp_color * bmp_color_byte;
  int const height = IMAGE_HEIGHT * PARTS_HEIGHT;
  int const width = IMAGE_WIDTH * PARTS_WIDTH;
  int const width_align = width + (width % 4);

  // BMPヘッダー読み込み
  bmp_header_t header;
  if (fread(&header, bmp_header_size, 1, fp) < 1) {
    fclose(fp);
    return NULL;
  }

  // 画像領域まで移動
  if (fseek(fp, header.offset, SEEK_SET) != 0) {
    fclose(fp);
    return NULL;
  }

  // バッファ生成
  uint8_t* buffer = (uint8_t*)malloc(header.image_size);
  if (buffer == NULL) {
    fclose(fp);
    return NULL;
  }

  // 画像領域読み込み
  if (fread(buffer, header.image_size, 1, fp) < 1) {
    free(buffer);
    fclose(fp);
    return NULL;
  }
  fclose(fp);

  // メモリ確保
  image_t* image = (image_t*)malloc(sizeof(image_t));
  if (image == NULL) {
    free(buffer);
    fclose(fp);
    return NULL;
  }

  // 画像情報の生成
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      parts_t * const parts = &(image->parts[iy][ix]);
      parts->locked = false;
      parts->no = iy * IMAGE_WIDTH + ix + 1;
      uint64_t sum = 0;
      for (int py = 0; py < PARTS_HEIGHT; ++ py) {
        for (int px = 0; px < PARTS_WIDTH; ++ px) {
          int const idx = (IMAGE_HEIGHT - iy - 1) * width_align * PARTS_HEIGHT +
                          (PARTS_HEIGHT - py - 1) * width_align + ix * PARTS_WIDTH + px;
          int const brightness = buffer[idx];
          parts->brightness[0][py][px] = (uint8_t)brightness;
          sum += brightness * brightness;
        }
      }
      // 90度回転した画像情報を生成
      for (int r = 1; r < ROTATION_SIZE; ++ r) {
        for (int py = 0; py < PARTS_HEIGHT; ++ py) {
          for (int px = 0; px < PARTS_WIDTH; ++ px) {
            parts->brightness[r][PARTS_HEIGHT - px - 1][py] = parts->brightness[r - 1][py][px];
          }
        }
      }
      // ベクトルの長さを求める
      double const dist = sqrt(sum);
      // 輝度の正規化
      for (int r = 0; r < ROTATION_SIZE; ++ r) {
        for (int py = 0; py < PARTS_HEIGHT; ++ py) {
          for (int px = 0; px < PARTS_WIDTH; ++ px) {
            parts->normalizing[r][py][px] = parts->brightness[r][py][px] / dist;
          }
        }
      }
    }
  }

  free(buffer);
  return image;
}

//////////////////////////////
// 画像オブジェクトからモザイクオブジェクトの生成
//////////////////////////////
mosaic_t* create_mosaic_by_image(image_t const* const image) {
  // メモリ確保
  mosaic_t* mosaic = (mosaic_t*)malloc(sizeof(mosaic_t));
  if (mosaic == NULL) {
    return NULL;
  }

  // パーツ関連付け
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      position_t* const position = &(mosaic->position[iy][ix]);
      position->rotation = 0;
      position->parts = &(image->parts[iy][ix]);
    }
  }

  return mosaic;
}

//////////////////////////////
// TXTからモザイクオブジェクトの生成
//////////////////////////////
mosaic_t* create_mosaic_by_txt(char const* const file_name, image_t const* const image) {
  FILE* fp = fopen(file_name, "r");
  if (fp == NULL) return NULL;

  // メモリ確保
  mosaic_t* mosaic = (mosaic_t*)malloc(sizeof(mosaic_t));
  if (mosaic == NULL) {
    return NULL;
  }

  // ファイル読み込み
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      position_t* const position = &(mosaic->position[iy][ix]);
      int no;
      if (fscanf(fp, "%d %d", &no, &(position->rotation)) == EOF) {
        free(mosaic);
        fclose(fp);
        return NULL;
      }
      int const py = (no - 1) / IMAGE_WIDTH;
      int const px = (no - 1) % IMAGE_WIDTH;
      position->parts = &(image->parts[py][px]);
    }
  }

  fclose(fp);
  return mosaic;
}

//////////////////////////////
// モザイクオブジェクトをTXTにエクスポート
//////////////////////////////
int export_mosaic_to_txt(char const* const file_name, mosaic_t const* const mosaic) {
  FILE* fp = fopen(file_name, "w");
  if (fp == NULL) return -1;

  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      position_t const* const position = &(mosaic->position[iy][ix]);
      if (fprintf(fp, "%d %d\n", position->parts->no, position->rotation) < 0) {
        fclose(fp);
        return -1;
      }
    }
  }

  fclose(fp);
  return 0;
}

//////////////////////////////
// モザイクオブジェクトをBMPにエクスポート
//////////////////////////////
int export_mosaic_to_bmp(char const* const file_name, mosaic_t const* const mosaic) {
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
    bmp_color_t* const ci = &(color[i]);
    ci->blue = i;
    ci->green = i;
    ci->red = i;
    ci->reserved = 0;
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
      position_t const* const position = &(mosaic->position[iy][ix]);
      for (int py = 0; py < PARTS_HEIGHT; ++ py) {
        for (int px = 0; px < PARTS_WIDTH; ++ px) {
          int const idx = (IMAGE_HEIGHT - iy - 1) * width_align * PARTS_HEIGHT +
                          (PARTS_HEIGHT - py - 1) * width_align + ix * PARTS_WIDTH + px;
          buffer[idx] = position->parts->brightness[position->rotation][py][px];
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

//////////////////////////////
// 画像オブジェクトをTXTにエクスポート
//////////////////////////////
int export_image_to_txt(char const* const file_name, image_t const* const image) {
  FILE* fp = fopen(file_name, "w");
  if (fp == NULL) return -1;

  // ファイル書き込み
  for (int iy = 0; iy < IMAGE_HEIGHT; ++ iy) {
    for (int ix = 0; ix < IMAGE_WIDTH; ++ ix) {
      parts_t const* const parts = &(image->parts[iy][ix]);
      if (fprintf(fp, "%d\n", parts->no) < 0) {
        fclose(fp);
        return -1;
      }
      for (int py = 0; py < PARTS_HEIGHT; ++ py) {
        if (fprintf(fp, "%d", parts->brightness[0][py][0]) < 0) {
          fclose(fp);
          return -1;
        }
        for (int px = 1; px < PARTS_WIDTH; ++ px) {
          if (fprintf(fp, " %d", parts->brightness[0][py][px]) < 0) {
            fclose(fp);
            return -1;
          }
        }
        if (fprintf(fp, "\n") < 0) {
          fclose(fp);
          return -1;
        }
      }
    }
  }

  fclose(fp);
  return 0;
}

//////////////////////////////
// 画像オブジェクトをBMPにエクスポート
//////////////////////////////
int export_image_to_bmp(char const* const file_name, image_t const* const image) {
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
    bmp_color_t* const ci = &(color[i]);
    ci->blue = i;
    ci->green = i;
    ci->red = i;
    ci->reserved = 0;
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
          buffer[idx] = parts->brightness[0][py][px];
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