#ifndef ENCRYPT_H
#define ENCRYPT_H

/* You must implement this function.
 * When the function returns the encryption module is allowed to reset.
 */
void reset_requested();
/* You must implement this function.
 * The function is called after the encryption module has finished a reset.
 */
void reset_finished();

/* You must use these functions to perform all I/O, encryption and counting
 * operations.
 */
void open_input(char *name);
void open_output(char *name);
int read_input();
void write_output(int c);
int caesar_encrypt(int c);
void count_input(int c);
void count_output(int c);
int get_input_count(int c);
int get_output_count(int c);

#endif // ENCRYPT_H
