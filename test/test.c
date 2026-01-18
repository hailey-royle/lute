#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "../src/le.h"

char* testString = "\nHello World!\n\nThis is my test string\nI hope it's good!\nExtra Line\nLast Line\n";
char* testPara = "one\n\nthree\nfour\n\nsix\nseven\n\neight\nnine\n\nten\n";
int testFailed = 0;
int testPassed = 0;

void Test(int res, char* msg) {
        if (res == 0) {
                ++testFailed;
                printf("\x1b[31m [ FAILED ] \x1b[0m %s\n", msg);
        } else {
                ++testPassed;
                printf("\x1b[32m [ PASSED ] \x1b[0m %s\n", msg);
        }
}

void InsertChars_Test() {
        {
                char* src = "1";
                int dstLen = 1;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '\0';
                InsertChars(&dst, &dstLen, 0, src, 1);
                Test(strcmp(dst, "1") == 0, "InsertChars blank dst and single src index 0");
                free(dst);
        }{
                char* src = "1";
                int dstLen = 2;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '2';
                dst[1] = '\0';
                InsertChars(&dst, &dstLen, 0, src, 1);
                Test(strcmp(dst, "12") == 0, "InsertChars single dst and single src index 0");
                free(dst);
        }{
                char* src = "1";
                int dstLen = 2;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '2';
                dst[1] = '\0';
                InsertChars(&dst, &dstLen, 1, src, 1);
                Test(strcmp(dst, "21") == 0, "InsertChars single dst and single src index 1");
                free(dst);
        }{
                char* src = "1";
                int dstLen = 3;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '2';
                dst[2] = '\0';
                InsertChars(&dst, &dstLen, 1, src, 1);
                Test(strcmp(dst, "012") == 0, "InsertChars single src index middle");
                free(dst);
        }{
                char* src = "01";
                int dstLen = 3;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '2';
                dst[1] = '3';
                dst[2] = '\0';
                InsertChars(&dst, &dstLen, 0, src, 2);
                Test(strcmp(dst, "0123") == 0, "InsertChars index start");
                free(dst);
        }{
                char* src = "12";
                int dstLen = 3;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '3';
                dst[2] = '\0';
                InsertChars(&dst, &dstLen, 1, src, 2);
                Test(strcmp(dst, "0123") == 0, "InsertChars index middle");
                free(dst);
        }{
                char* src = "23";
                int dstLen = 3;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '1';
                dst[2] = '\0';
                InsertChars(&dst, &dstLen, 2, src, 2);
                Test(strcmp(dst, "0123") == 0, "InsertChars index end");
                free(dst);
        }
}

void DeleteChars_Test() {
        {
                int dstLen = 4;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '1';
                dst[2] = '2';
                dst[3] = '\0';
                DeleteChars(&dst, &dstLen, 1, 0);
                Test(strcmp(dst, "012") == 0, "DeleteChars count 0");
                free(dst);
        }{
                int dstLen = 4;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '1';
                dst[2] = '2';
                dst[3] = '\0';
                DeleteChars(&dst, &dstLen, 3, 1);
                Test(strcmp(dst, "01") == 0, "DeleteChars end");
                free(dst);
        }{
                int dstLen = 4;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '1';
                dst[2] = '2';
                dst[3] = '\0';
                DeleteChars(&dst, &dstLen, 2, 1);
                Test(strcmp(dst, "02") == 0, "DeleteChars middle");
                free(dst);
        }{
                int dstLen = 4;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '1';
                dst[2] = '2';
                dst[3] = '\0';
                DeleteChars(&dst, &dstLen, 1, 1);
                Test(strcmp(dst, "12") == 0, "DeleteChars start");
                free(dst);
        }{
                int dstLen = 4;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '1';
                dst[2] = '2';
                dst[3] = '\0';
                DeleteChars(&dst, &dstLen, 3, 2);
                Test(strcmp(dst, "0") == 0, "DeleteChars count 2");
                free(dst);
        }{
                int dstLen = 4;
                char* dst = malloc(dstLen);
                assert(dst != NULL);
                dst[0] = '0';
                dst[1] = '1';
                dst[2] = '2';
                dst[3] = '\0';
                DeleteChars(&dst, &dstLen, 3, 3);
                Test(strcmp(dst, "") == 0, "DeleteChars all");
                free(dst);
        }
}

void PrevCharIndex_Test() {
        Test(PrevCharIndex(&testString, strlen(testString), 0, 0) == 0, "PrevCharIndex count 0");
        Test(PrevCharIndex(&testString, strlen(testString), 1, 1) == -1, "PrevCharIndex count 1");
        Test(PrevCharIndex(&testString, strlen(testString), 10, 10) == -10, "PrevCharIndex count 10");
        Test(PrevCharIndex(&testString, strlen(testString), 0, 1) == 0, "PrevCharIndex count overflow");
        Test(PrevCharIndex(&testString, strlen(testString), 1, 2) == -1, "PrevCharIndex count + index overflow");
        Test(PrevCharIndex(&testString, strlen(testString), strlen(testString) - 1, 1) == -1, "PrevCharIndex index max");
}

void NextCharIndex_Test() {
        Test(NextCharIndex(&testString, strlen(testString), 0, 0) == 0, "NextCharIndex count 0");
        Test(NextCharIndex(&testString, strlen(testString), 0, 1) == 1, "NextCharIndex count 1");
        Test(NextCharIndex(&testString, strlen(testString), 0, 10) == 10, "NextCharIndex count 10");
        Test(NextCharIndex(&testString, strlen(testString), 0, 1000) == strlen(testString) - 1, "NextCharIndex count overflow");
        Test(NextCharIndex(&testString, strlen(testString), 40, 40) == strlen(testString) - 1 - 40, "NextCharIndex count + index overflow");
        Test(NextCharIndex(&testString, strlen(testString), strlen(testString) - 1, 1) == 0, "NextCharIndex index max");
}

void PrevWordIndex_Test() {
        Test(PrevWordIndex(&testString, strlen(testString), 0, 0) == 0, "PrevWordIndex count 0");
        Test(PrevWordIndex(&testString, strlen(testString), 52, 1) == -3, "PrevWordIndex count 1");
        Test(PrevWordIndex(&testString, strlen(testString), 52, 2) == -8, "PrevWordIndex count 2");
        Test(PrevWordIndex(&testString, strlen(testString), 52, 3) == -13, "PrevWordIndex count 3");
        Test(PrevWordIndex(&testString, strlen(testString), 15, 3) == -9, "PrevWordIndex count 3 space and newline");
        Test(PrevWordIndex(&testString, strlen(testString), 0, 1) == 0, "PrevWordIndex count overflow");
        Test(PrevWordIndex(&testString, strlen(testString), 1, 1) == -1, "PrevWordIndex count + index overflow");
        Test(PrevWordIndex(&testString, strlen(testString), strlen(testString) - 1, 1) == -5, "PrevWordIndex index max");
}

void NextWordIndex_Test() {
        Test(NextWordIndex(&testString, strlen(testString), 0, 0) == 0, "NextWordIndex count 0");
        Test(NextWordIndex(&testString, strlen(testString), 52, 1) == 3, "NextWordIndex count 1");
        Test(NextWordIndex(&testString, strlen(testString), 52, 2) == 9, "NextWordIndex count 2");
        Test(NextWordIndex(&testString, strlen(testString), 52, 3) == 14, "NextWordIndex count 3");
        Test(NextWordIndex(&testString, strlen(testString), 15, 3) == 10, "NextWordIndex count 3 space and newline");
        Test(NextWordIndex(&testString, strlen(testString), strlen(testString) - 1, 1) == 0, "NextWordIndex count overflow");
        Test(NextWordIndex(&testString, strlen(testString), strlen(testString) - 2, 1) == 1, "NextWordIndex count + index overflow");
}

void PrevLineIndex_Test() {
        Test(PrevLineIndex(&testString, strlen(testString), 0, 0) == 0, "PrevLineIndex count 0");
        Test(PrevLineIndex(&testString, strlen(testString), 52, 1) == -37, "PrevLineIndex count 1");
        Test(PrevLineIndex(&testString, strlen(testString), 52, 2) == -38, "PrevLineIndex count 2");
        Test(PrevLineIndex(&testString, strlen(testString), 52, 3) == -51, "PrevLineIndex count 3");
        Test(PrevLineIndex(&testString, strlen(testString), 0, 1) == 0, "PrevLineIndex count overflow");
        Test(PrevLineIndex(&testString, strlen(testString), 1, 1) == -1, "PrevLineIndex count + index overflow");
        Test(PrevLineIndex(&testString, strlen(testString), strlen(testString) - 1, 1) == -20, "PrevLineIndex index max");
}

void NextLineIndex_Test() {
        Test(NextLineIndex(&testString, strlen(testString), 0, 0) == 1, "NextLineIndex count 0");
        Test(NextLineIndex(&testString, strlen(testString), 30, 1) == 8, "NextLineIndex count 1");
        Test(NextLineIndex(&testString, strlen(testString), 30, 2) == 26, "NextLineIndex count 2");
        Test(NextLineIndex(&testString, strlen(testString), 30, 3) == 37, "NextLineIndex count 3");
        Test(NextLineIndex(&testString, strlen(testString), 0, 1000) == strlen(testString) - 1, "NextLineIndex count overflow");
        Test(NextLineIndex(&testString, strlen(testString), strlen(testString) - 2, 1) == 1, "NextLineIndex count + index overflow");
}

void PrevParaIndex_Test() {
        Test(PrevParaIndex(&testPara, strlen(testPara), 0, 0) == 0, "PrevParaIndex count 0");
        Test(PrevParaIndex(&testPara, strlen(testPara), 40, 1) == -1, "PrevParaIndex count 1");
        Test(PrevParaIndex(&testPara, strlen(testPara), 40, 2) == -13, "PrevParaIndex count 2");
        Test(PrevParaIndex(&testPara, strlen(testPara), 40, 3) == -24, "PrevParaIndex count 3");
        Test(PrevParaIndex(&testPara, strlen(testPara), 0, 1) == 0, "PrevParaIndex count overflow");
        Test(PrevParaIndex(&testPara, strlen(testPara), 40, 1000) == -40, "PrevParaIndex count + index overflow");
        Test(PrevParaIndex(&testPara, strlen(testPara), strlen(testPara) - 1, 1) == -4, "PrevParaIndex index max");
}

void NextParaIndex_Test() {
        Test(NextParaIndex(&testPara, strlen(testPara), 0, 0) == 0, "NextParaIndex count 0");
        Test(NextParaIndex(&testPara, strlen(testPara), 0, 1) == 4, "NextParaIndex count 1");
        Test(NextParaIndex(&testPara, strlen(testPara), 0, 2) == 16, "NextParaIndex count 2");
        Test(NextParaIndex(&testPara, strlen(testPara), 0, 3) == 27, "NextParaIndex count 3");
        Test(NextParaIndex(&testPara, strlen(testPara), 0, 1000) == strlen(testPara) - 1, "NextParaIndex count overflow");
        Test(NextParaIndex(&testPara, strlen(testPara), strlen(testPara) - 2, 1) == 1, "NextParaIndex count + index overflow");
}

void StartLineIndex_Test() {
        Test(StartLineIndex(&testString, strlen(testString), 0) == 0, "StartStringIndex index 0");
        Test(StartLineIndex(&testString, strlen(testString), 1) == 0, "StartStringIndex start of line");
        Test(StartLineIndex(&testString, strlen(testString), 2) == -1, "StartStringIndex middle line");
}

void EndLineIndex_Test() {
        Test(EndLineIndex(&testString, strlen(testString), 0) == 0, "EndStringIndex index 0");
        Test(EndLineIndex(&testString, strlen(testString), 14) == 0, "EndStringIndex end of line");
        Test(EndLineIndex(&testString, strlen(testString), 10) == 3, "EndStringIndex middle line");
}

void StartFileIndex_Test() {
        Test(StartFileIndex(&testString, strlen(testString), 0) == 0, "StartFileIndex index 0");
        Test(StartFileIndex(&testString, strlen(testString), 20) == -20, "StartFileIndex file middle");
}

void EndFileIndex_Test() {
        Test(EndFileIndex(&testString, strlen(testString), 0) == strlen(testString) - 1, "EndFileIndex index 0");
        Test(EndFileIndex(&testString, strlen(testString), 20) == strlen(testString) - 1 - 20, "EndFileIndex file middle");
}

void LineIndex_Test() {
        Test(LineIndex(&testString, strlen(testString), 0, 0) == 0, "LineIndex index 0 line 0");
        Test(LineIndex(&testString, strlen(testString), 0, 1) == 1, "LineIndex index 0 line 1");
        Test(LineIndex(&testString, strlen(testString), 0, 6) == 67, "LineIndex index 0 line last");
        Test(LineIndex(&testString, strlen(testString), 0, 7) == 76, "LineIndex index 0 line last + 1");
        Test(LineIndex(&testString, strlen(testString), 0, 100) == 76, "LineIndex index 0 line overflow");
        Test(LineIndex(&testString, strlen(testString), 10, 0) == -10, "LineIndex line 0");
        Test(LineIndex(&testString, strlen(testString), 10, 1) == -9, "LineIndex line 1");
        Test(LineIndex(&testString, strlen(testString), 10, 6) == 57, "LineIndex line last");
        Test(LineIndex(&testString, strlen(testString), 10, 7) == 66, "LineIndex line last + 1");
        Test(LineIndex(&testString, strlen(testString), 10, 100) == 66, "LineIndex line overflow");
        Test(LineIndex(&testString, strlen(testString), 1, 1) == 0, "LineIndex index = result");
}

void LineAbsolute_Test() {
        Test(LineAbsolute(&testString, strlen(testString), 0) == 0, "LineAbsolute line 0");
        Test(LineAbsolute(&testString, strlen(testString), 1) == 1, "LineAbsolute line 1");
        Test(LineAbsolute(&testString, strlen(testString), 2) == 14, "LineAbsolute blank line");
        Test(LineAbsolute(&testString, strlen(testString), 3) == 15, "LineAbsolute after blank line");
        Test(LineAbsolute(&testString, strlen(testString), 6) == 67, "LineAbsolute last line");
        Test(LineAbsolute(&testString, strlen(testString), 7) == 76, "LineAbsolute last line + 1");
        Test(LineAbsolute(&testString, strlen(testString), 100) == 76, "LineAbsolute line overflow");
}

void TestResult() {
        printf("\x1b[31m [ FAILED : %d ] \x1b[32m[ PASSED : %d ]\x1b[0m\n", testFailed, testPassed);
        if (testFailed > 0) {
                for (int i = 0; i < strlen(testString); ++i) printf("%d:%c\n", i, testString[i]);
                printf(testString);
        }
}

int main() {
        InsertChars_Test();
        DeleteChars_Test();
        PrevCharIndex_Test();
        NextCharIndex_Test();
        PrevWordIndex_Test();
        NextWordIndex_Test();
        PrevLineIndex_Test();
        NextLineIndex_Test();
        PrevParaIndex_Test();
        NextParaIndex_Test();
        StartLineIndex_Test();
        EndLineIndex_Test();
        StartFileIndex_Test();
        EndFileIndex_Test();
        LineIndex_Test();
        LineAbsolute_Test();
        TestResult();
}
