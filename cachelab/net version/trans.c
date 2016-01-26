//
//  trans.c
//  csim
//
//  Created by 胡家兴 on 10/13/15.
//  Copyright © 2015 胡家兴. All rights reserved.
//
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
    

    
    // when N == 32 && M ==32
    if (N == 32)
    {
        // first double forloop: for each block
        for (block_c = 0; block_c < N; block_c += 8) {
            for (block_r = 0; block_r < N; block_r += 8) {
                // second double for loop: for each element in block
                for (i = block_r; i < block_r + 8; i ++) {
                    for (j = block_c; j < block_c + 8; j ++) {
                        // if element is not on diagonal, transpose to correct position in B.
                        // if element is on diagonal, store in tmp variable
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            t = A[i][j];
                        }
                    }
                    // write the temp variable (on diagonal) to B matrix
                    if (block_r == block_c) {
                        B[i][i] = t;
                    }
                }
            }
        }
    }
    
    // when N == 64 && M ==64
    else if (N == 64) {
        for (block_c = 0; block_c < N; block_c += 4) {
            for (block_r = 0; block_r < N; block_r += 4) {
                for (i = block_r; i < block_r + 4; i ++) {
                    for (j = block_c; j < block_c + 4; j ++) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            t = A[i][j];
                        }
                    }
                    if (block_r == block_c) {
                        B[i][i] = t;
                    }
                }
            }
        }
    }
    // when N == 67 && M == 61
    else {
        for (block_c = 0; block_c < M; block_c += 16) {
            for (block_r = 0; block_r < N; block_r += 16) {
                for (i = block_r; (i < block_r + 16) && (i < N); i ++) {
                    for (j = block_c; (j < block_c + 16) && (j < M); j ++) {
                        if (i != j) {
                            B[j][i] = A[i][j];
                        } else {
                            t = A[i][j];
                        }
                    }
                    if (block_r == block_c) {
                        B[i][i] = t;
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

