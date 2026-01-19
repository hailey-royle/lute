#ifndef LEH
#define LEH

void InsertChars(char** dst, int *dstLen, int index, char* src, int count);
void DeleteChars(char** dst, int *dstLen, int index, int count);
int PrevCharIndex(char** text, int textLen, int index, int count);
int NextCharIndex(char** text, int textLen, int index, int count);
int PrevWordIndex(char** text, int textLen, int index, int count);
int NextWordIndex(char** text, int textLen, int index, int count);
int PrevLineIndex(char** text, int textLen, int index, int count);
int NextLineIndex(char** text, int textLen, int index, int count);
int PrevParaIndex(char** text, int textLen, int index, int count);
int NextParaIndex(char** text, int textLen, int index, int count);
int StartLineIndex(char** text, int textLen, int index);
int EndLineIndex(char** text, int textLen, int index);
int StartFileIndex(char** text, int textLen, int index);
int EndFileIndex(char** text, int textLen, int index);
int LineIndex(char** text, int textLen, int index, int line);
int LineAbsolute(char** text, int textLen, int line);
int LineNumber(char** text, int textLen, int index);
//int FindStringIndex(char** text, int textLen, int count);

void InsertChars(char** dst, int *dstLen, int index, char* src, int count) {
        assert(dst != NULL);
        assert(*dst != NULL);
        assert(dstLen != NULL);
        assert(*dstLen >= 0);
        assert(src != NULL);
        assert(index >= 0);
        assert(count >= 0);
        if (count == 0) {
                return;
        }
        char* tmp = realloc(*dst, *dstLen + count);
        assert(tmp != NULL);
        tmp[*dstLen + count - 1] = '\0';
        memmove(&tmp[index + count], &tmp[index], *dstLen - 1 - index);
        memmove(&tmp[index], src, count);
        *dst = tmp;
        *dstLen += count;
}

void DeleteChars(char** dst, int *dstLen, int index, int count) {
        assert(dst != NULL);
        assert(*dst != NULL);
        assert(dstLen != NULL);
        assert(*dstLen >= 0);
        assert(index >= 0);
        assert(count >= 0);
        if (count > index) {
                count = index;
        }
        if (count < 0) {
                return;
        }
        char* tmp = *dst;
        memmove(&tmp[index - count], &tmp[index], *dstLen - index);
        tmp = realloc(*dst, *dstLen - count);
        assert(tmp != NULL);
        tmp[*dstLen - count - 1] = '\0';
        *dst = tmp;
        *dstLen -= count;
}

int PrevCharIndex(char** text, int textLen, int index, int count) {
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        assert(count >= 0);
        if (count == 0) return 0;
        if (count > index) return 0 - index;
        return ~(count - 1);
}

int NextCharIndex(char** text, int textLen, int index, int count) {
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        assert(count >= 0);
        if (count == 0) return 0;
        if (count + index >= textLen - 1) return textLen - 1 - index;
        return count;
}

int PrevWordIndex(char** text, int textLen, int index, int count) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        assert(count >= 0);
        while (count > 0) {
                if (index + ret <= 0) {
                        break;
                }
                --ret;
                if (*(*text + index + ret) == ' ' || *(*text + index + ret) == '\n') {
                        --count;
                }
        }
        return ret;
}

int NextWordIndex(char** text, int textLen, int index, int count) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        assert(count >= 0);
        while (count > 0) {
                if (index + ret >= textLen - 1) {
                        break;
                }
                ++ret;
                if (*(*text + index + ret) == ' ' || *(*text + index + ret) == '\n') {
                        --count;
                }
        }
        return ret;
}

int PrevLineIndex(char** text, int textLen, int index, int count) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        assert(count >= 0);
        while (count > 0) {
                if (index + ret <= 0) {
                        break;
                }
                --ret;
                if (*(*text + index + ret) == '\n') {
                        --count;
                }
        }
        while (index + ret > 0) {
                --ret;
                if (*(*text + index + ret) == '\n') {
                        ++ret;
                        break;
                }
        }
        return ret;
}

int NextLineIndex(char** text, int textLen, int index, int count) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        assert(count >= 0);
        while (count > 0) {
                if (index + ret >= textLen - 1) {
                        break;
                }
                if (*(*text + index + ret) == '\n') {
                        --count;
                }
                ++ret;
        }
        return ret;
}

int PrevParaIndex(char** text, int textLen, int index, int count) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        assert(count >= 0);
        while (count > 0) {
                if (index + ret <= 0) {
                        break;
                }
                --ret;
                if (*(*text + index + ret) == '\n') {
                        if (index + ret <= 0) {
                                break;
                        }
                        --ret;
                        if (*(*text + index + ret) == '\n') {
                                ++ret;
                                --count;
                        }
                }
        }
        return ret;
}

int NextParaIndex(char** text, int textLen, int index, int count) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        assert(count >= 0);
        while (count > 0) {
                if (index + ret >= textLen - 1) {
                        break;
                }
                ++ret;
                if (*(*text + index + ret) == '\n') {
                        if (index + ret >= textLen - 1) {
                                break;
                        }
                        ++ret;
                        if (*(*text + index + ret) == '\n') {
                                --count;
                        }
                }
        }
        return ret;
}

int StartLineIndex(char** text, int textLen, int index) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        while (index + ret > 0) {
                --ret;
                if (*(*text + index + ret) == '\n') {
                        ++ret;
                        break;
                }
        }
        return ret;
}

int EndLineIndex(char** text, int textLen, int index) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        while (index + ret < textLen) {
                if (*(*text + index + ret) == '\n') {
                        break;
                }
                ++ret;
        }
        return ret;
}

int StartFileIndex(char** text, int textLen, int index) {
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        return 0 - index;
}

int EndFileIndex(char** text, int textLen, int index) {
        assert(text != NULL);
        assert(textLen >= 0);
        assert(textLen > index);
        assert(index >= 0);
        return textLen - 1 - index;
}

int LineIndex(char** text, int textLen, int index, int line) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(index >= 0);
        assert(index < textLen);
        assert(line >= 0);
        if (line == 0) {
                return 0 - index;
        }
        while (line > 0) {
                if (ret >= textLen - 1) {
                        break;
                }
                if (*(*text + ret) == '\n') {
                        --line;
                }
                ++ret;
        }
        return ret - index;
}

int LineAbsolute(char** text, int textLen, int line) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(line >= 0);
        if (line == 0) {
                return 0;
        }
        while (line > 0) {
                if (ret >= textLen - 1) {
                        return -1;
                }
                if (*(*text + ret) == '\n') {
                        --line;
                }
                ++ret;
        }
        return ret;
}

int LineNumber(char** text, int textLen, int index) {
        int ret = 0;
        assert(text != NULL);
        assert(textLen >= 0);
        assert(index >= 0);
        assert(index < textLen);
        while (index > 0) {
                if (ret >= textLen - 1) {
                        break;
                }
                --index;
                if (*(*text + index) == '\n') {
                        ++ret;
                }
        }
        return ret;
}

/*int FindStringIndex(char** text, int textLen, int count) {
        assert(false);
}*/

#endif
