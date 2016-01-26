/*
 * Name: Jiaxing Hu
 * Andrew ID: jiaxingh
*/
/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, b_i, b_j;
    int temp = 0;//used to save the transpose value so that this line doesn't need to be evicted
    int temp1,temp2,temp3;//used as temperary place to save matrix.

    
    // when N == 32 && M ==32
    if (N == 32)
    {
        /*iterate every block(4 * 4)
         because A starts at 6022c0 and B starts at 6422c0, when it is the transpose condiction,
         the line will be overwrote. So I eject this special condition.
         and because m is 5, one line can contain at most 8 integer(2 + 3 = 5)
         so every time when the program need one integer in matrix, it will put eight integers into the line. 
         Because this is a 32 * 32 matrix, the set will be evicted every 8 rows.
         Therefore, the bsize should be 8.*/
        for (b_j = 0; b_j < N; b_j += 8) {
            for (b_i = 0; b_i < N; b_i += 8) {
                for (i = b_i; i < b_i + 8; i ++) {
                    for (j = b_j; j < b_j + 8; j ++) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            temp = A[i][j];
                        }
                    }
                    if (b_i == b_j) {
                        B[i][i] = temp;
                    }
                }
            }
        }
    }
    
    // when N == 64 && M ==64
    else if(N == 64 && M == 64){
        /*iterate every block(16 * 16)
         because A starts at 6022c0 and B starts at 6422c0.
         Because this is a 64 * 64 matrix, the set will be evicted every 4 rows.
         Therefore, the bsize should be 4.
         and to solve the conflict missing, we can use temp to save the value*/
        for (j = 0; j < M; j+=8) {
            for (i = 0; i < N; i+=8) {//for one 8*8 block, it can be divided into four parts, which can be represents as (0,0),(0,1),(1,0),(1,1)
                for (b_i = i; b_i < i+4; b_i++) {
                    //A(0,0) -> B(0,0)
                    B[j][b_i] = A[b_i][j];
                    B[j+1][b_i] = A[b_i][j+1];
                    B[j+2][b_i] = A[b_i][j+2];
                    B[j+3][b_i] = A[b_i][j+3];
                    
                    b_j++;
                    //A(0,1) -> B(0,1)   B(0,1) is the temporary storage
                    temp = A[b_i][j+4];
                    temp1 = A[b_i][j+5];
                    temp2 = A[b_i][j+6];
                    temp3 = A[b_i][j+7];
                    
                    B[j][b_i+4] = temp;
                    B[j+1][b_i+4] = temp1;
                    B[j+2][b_i+4] = temp2;
                    B[j+3][b_i+4] = temp3;
                }
                
                
                for (b_i = j+4; b_i < j+8; b_i++) {
                    //A(0,1) -> B(0,1) -> temp
                    temp = B[b_i-4][i+4];
                    temp1 = B[b_i-4][i+5];
                    temp2 = B[b_i-4][i+6];
                    temp3 = B[b_i-4][i+7];
                    
                    b_j++;
                    ////A(1,0) -> B(0,1)
                    B[b_i-4][i+4] = A[i+4][b_i-4];
                    B[b_i-4][i+5] = A[i+5][b_i-4];
                    B[b_i-4][i+6] = A[i+6][b_i-4];
                    B[b_i-4][i+7] = A[i+7][b_i-4];
                    
                    b_j++;
                    //temp -> B(1,0)
                    B[b_i][i] = temp;
                    B[b_i][i+1] = temp1;
                    B[b_i][i+2] = temp2;
                    B[b_i][i+3] = temp3;
                    
                    b_j++;
                    //(1, 1) -> (1, 1)
                    B[b_i][i+4] = A[i+4][b_i];
                    B[b_i][i+5] = A[i+5][b_i];
                    B[b_i][i+6] = A[i+6][b_i];
                    B[b_i][i+7] = A[i+7][b_i];
                }
            }
            
        }

    }
    
    // when N == 67 && M == 61
        /*iterate every block
         because A starts at 6022c0 and B starts at 6422c0.
         it is a 61 * 67 matrix, so every time when the pointer moves to next row of A, we will have extra 3 integer.
         and after 16 rows, we have 48 integers(0x1100 0000), and at this time, I found that the cache begin a new set.
         Therefore, the bsize should be 16.*/
    else {
        for (b_j = 0; b_j < M; b_j += 16) {
            for (b_i = 0; b_i < N; b_i += 16) {
                for (i = b_i; (i < b_i + 16) && (i < N); i ++) {
                    for (j = b_j; (j < b_j + 16) && (j < M); j ++) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            temp = A[i][j];
                        }
                    }
                    if (b_i == b_j) {
                        B[i][i] = temp;
                    }
                }
            }
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;
    
    REQUIRES(M > 0);
    REQUIRES(N > 0);
    
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
    
    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);
    
    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
    
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    
    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

