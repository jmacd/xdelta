/* Windows compatibility definitions for xdelta3 */
#ifndef _XDELTA3_WIN32_H_
#define _XDELTA3_WIN32_H_

#ifdef _WIN32

/* Define Windows-specific stubs for UNIX functions */
typedef int pid_t;

/* Define stub macros for Windows */
#define PIPE_READ_FD  0
#define PIPE_WRITE_FD 1
#define MAX_SUBPROCS  4

/* Windows stubs for UNIX functions */
static int main_waitpid_check(int pid) { return 0; }
static int main_external_compression_finish(void) { return 0; }
static void main_external_compression_cleanup(void) {}
static int main_pipe_write(int outfd, uint8_t *exist_buf, usize_t remain) { return 0; }
static int main_pipe_copier(uint8_t *pipe_buf, usize_t pipe_bufsize,
                           size_t nread, main_file *ifile, int outfd) { return 0; }
static int main_input_decompress_setup(const main_extcomp *decomp,
                                      main_file *ifile,
                                      uint8_t *input_buf,
                                      usize_t input_bufsize,
                                      uint8_t *pipe_buf,
                                      usize_t pipe_bufsize,
                                      usize_t pipe_avail,
                                      size_t *nread)
{
  XPR(NT "External decompression not supported on Windows\n");
  return -1;
}
static int main_recompress_output(main_file *ofile)
{
  XPR(NT "External recompression not supported on Windows\n");
  return -1;
}

#endif /* _WIN32 */

#endif /* _XDELTA3_WIN32_H_ */
