#ifndef LCD1602_H_
#define LCD1602_H_

#include <esp_log.h>

#ifdef __cplusplus
    extern "C" {
#endif

esp_err_t i2c_master_init(void);
void lcd_send_cmd (char cmd);
void lcd_send_data(char data);
void lcd_init (void);
void lcd_put_cur(int row, int col);
void lcd_send_string(char *str);
void lcd_clear(void);

/* *INDENT-OFF* */
#ifdef __cplusplus
    }
#endif
/* *INDENT-ON* */

#endif