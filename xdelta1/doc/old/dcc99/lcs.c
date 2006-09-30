/* * Last edited: Jun  9 11:06 1997 (jjh) */
/* computes the length of the longest common subsequence of two files, 
   byte by byte. Outputs the length of the two files, the length
   of the LCS, and the fragmentation of both files.
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <malloc.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

main(int argc, char *argv[])
{
  FILE *file1, *file2;
  struct stat file1_stat, file2_stat;
  off_t j, file1_size, file2_size;
  off_t file1_fragmentation, file2_fragmentation;
  int fileswitched; /* indicates whether the order of files was changed.*/

  unsigned char *file1_data;
  off_t *LCS;
  off_t oldbest;
  register off_t newbest;
  register int ci;

  if (argc != 3) 
    {
      fprintf(stderr, "%s file1 file2\n", argv[0]);
      fprintf(stderr, "outputs on stdout the lengths of file1 and file2, \n");
      fprintf(stderr, "followed by the length of the ");
      fprintf(stderr, "longest common subsequence of file1 and file2,\n");
      fprintf(stderr, "followed by the fragmentation (number of segments)\n");
      fprintf(stderr,
	      "induced by the longest common subsequence on both files.\n");
      exit(EXIT_FAILURE);
    }
  else if ((file1 = fopen(argv[1],"r")) == NULL) 
    {
      fprintf(stderr, "Can't open %s\n", argv[1]);
      exit(EXIT_FAILURE);
    }
  else if ((file2 = fopen(argv[2],"r")) == NULL) 
    {
      fprintf(stderr,"Can't open %s\n",argv[2]);
      exit(EXIT_FAILURE);
    }

  fstat(fileno(file1),&file1_stat);
  fstat(fileno(file2),&file2_stat);
  file1_size=file1_stat.st_size;
  file2_size=file2_stat.st_size;

  if (file1_stat.st_size > file2_stat.st_size) 
    {
      /* swap file to save space--size of file1 determines the space */
      FILE *filetmp;
      off_t filetmp_size;
      filetmp = file1; file1 = file2; file2 = filetmp;
      filetmp_size = file1_size; file1_size = file2_size; 
      file2_size = filetmp_size;
      fileswitched = 1;
    } 
  else
    {
      fileswitched = 0;
    }

  if ((NULL == (LCS = (off_t *)malloc((unsigned)((file1_size + 1) *
						 sizeof(off_t))))))
    {
      fprintf(stderr,"Can't allocate enough space (%d bytes needed)\n",
	      (file1_size + 1)*sizeof(off_t));
      exit(EXIT_FAILURE);
    }
  else if (NULL == (file1_data=(unsigned char *)malloc((unsigned)file1_size)))
    {
      fprintf(stderr,"Can't allocate enough space to load file.\n");
      exit(EXIT_FAILURE);
    }
  else if (fread(file1_data, 1, file1_size, file1) != file1_size) 
    {
      fprintf(stderr, "File reading failed\n");
      exit(EXIT_FAILURE);
    }

  /* here starts the LCS algorithm */
    

  /* Initialize the array for computing the length */
  for (j = 0; j <= file1_size; j++) LCS[j] = 0;
  file1_fragmentation = file2_fragmentation = 1;
  oldbest = 1;  /* sentinel to control counting of fragmentation */

# ifdef DEBUG
  printf("     ");
  for (j = 0; j <= file1_size; j++)  printf("%3c", file1_data[j]);
  printf("  segments\n  ");
  for (j = 0; j <= file1_size; j++) printf("%3d", LCS[j]);
  putchar('\n');
# endif    
    
  while ((ci = getc(file2)) != EOF) 
    {
      register off_t *jptr;
      register unsigned char *current, *stop;

      newbest = 0;
      jptr = LCS;
#     ifdef DEBUG
      printf("%c %3d",ci,newbest);
#     endif
      for (current = file1_data, stop = file1_data + file1_size;
	   current < stop;
	   current++)
	{
	  register off_t extendlength;
	  extendlength = *jptr + (ci ==  *current);
	  *jptr = newbest; /* write best value from last iter. */
	  if (newbest < extendlength) newbest = extendlength;
	  jptr++;
	  if (newbest < *jptr) newbest = *jptr;
#         ifdef DEBUG
	  printf("%3d", newbest);
#         endif
	}
	    
      /* update fragmentation of file2 */
      if (((*jptr == newbest) && (oldbest<*jptr)) ||
	  ((*jptr < newbest) && (oldbest == *jptr)))
	file2_fragmentation++;

      oldbest = *jptr;
      *jptr = newbest;
#     ifdef DEBUG
      printf("   %3d\n", file2_fragmentation);
#     endif
    }

  /* determine fragmentation of file1*/
  for (j = 1;j<file1_size;j++)
    {
      if (((LCS[j] == LCS[j + 1]) && (LCS[j - 1] != LCS[j])) ||
	  ((LCS[j] != LCS[j + 1]) && (LCS[j - 1] == LCS[j])))
	file1_fragmentation++;
    }

  if (!fileswitched) 
    printf("%d %d %d %d %d\n", file1_size, file2_size,
	   newbest, file1_fragmentation, file2_fragmentation); 
  else
    printf("%d %d %d %d %d\n", file2_size, file1_size,
	   newbest, file2_fragmentation, file1_fragmentation); 

  exit(EXIT_SUCCESS);
}
