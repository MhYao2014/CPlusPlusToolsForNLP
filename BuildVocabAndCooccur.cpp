//
// Created by mhyao on 19-11-26.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#define TSIZE 1048576
#define SEED  1159241
#define MaxWordLen 1000
#define HASHFN  HashValue

typedef struct VocabHashWithId {
    char *Word;
    long long Count;
    long long id;
    struct VocabHashWithId *next;
} HASHUNITID;

typedef struct VocabUnitHash {
    char *Word;
    long long Count;
    struct VocabUnitHash *next;
} HASHUNIT;

typedef struct VocabUnit {
    char *Word;
    long long Count;
} ARRAYUNIT;

int scmp(char *s1, char *s2) {
    // 以s1字符串为标准，看s2是不是等于s1
    // 两个字符串不相同时，返回1；
    // 相同时，返回0。
    while (*s1 != '\0' && *s1 == *s2) {s1++;s2++;}
    return *s1 - *s2;
}

HASHUNIT ** InitHashTable(int Tsize) {
    HASHUNIT **ht = (HASHUNIT **) malloc(sizeof(HASHUNIT *) * Tsize);
    for (int i=0;i<Tsize;i++) {
        ht[i] = (HASHUNIT *) NULL;
    }
    return ht;
}

HASHUNITID ** InitHashTableID(int Tsize) {
    HASHUNITID **ht = (HASHUNITID **) malloc(sizeof(HASHUNITID *) * Tsize);
    for (int i=0;i<Tsize;i++) {
        ht[i] = (HASHUNITID *) NULL;
    }
    return ht;
}

unsigned int HashValue(char *word, int tsize, unsigned int seed) {
    char c;
    unsigned int h;
    h = seed;
    for ( ; (c = *word) != '\0'; word++) h ^= ((h << 5) + c + (h >> 2));
    return (unsigned int)((h & 0x7fffffff) % tsize);
}

void HashMapWord(char *Word, HASHUNITID **VocabHash) {
    HASHUNITID *hpre= NULL, *hnow= NULL;
    // 首先计算Word字符串的哈希值
    unsigned int hval = HASHFN(Word, TSIZE, SEED);
    // 然后检查是否哈希冲突(当前是否为空，是否和Word内容一致)，
    // 若冲突则用链表解决:指向链表的下一个单元。
    for (   hpre = NULL,hnow = VocabHash[hval];
            hnow != NULL && scmp(hnow->Word, Word)!=0;
            hpre = hnow, hnow = hnow->next );
    // 此时hnow要么为空，要么和Word内容相同，
    // 和Word内容相同时，那就让hnow指向的结构体中的Count+1。
    // 当hnow为空的时候就新开一个小内存，并把这块小内存的地址给hnow,
    // 最后把Word的内容给这块新的内存，Count+1,next设为NULL(让这块新内存变为链表的最后一个节点)。
    if (hnow == NULL) {
        // hnow为空表明该指针没有指向任何内存，所以需要先开辟内存
        // 然后把hnow指向这个内存。
        hnow = (HASHUNITID *) malloc(sizeof(HASHUNITID));
        hnow->Word = (char *) malloc(strlen(Word) + 1);
        strcpy(hnow->Word, Word);
        hnow->Count = 1;
        hnow->id = -1;
        hnow->next = NULL;
        if (hpre == NULL) {
            // hnow指向的那块内存是VocabHash[hval]处的第一个节点
            VocabHash[hval] = hnow;
        } else {
            // 将hnow接到hpre后面
            hpre->next = hnow;
        }
    } else {
        // hnow不为空，就说明遇到同一个词了
        // 将该词Count+1，同时将hnow指向的内存挂在链表的第一个节点
        hnow->Count++;
        // 先判断hnow所指向的内存在链表中是不是已经处在第一个节点了？
        // 如果是的话(表现为hpre指向NULL)，那就不用再移动了;
        // 如果不是的话(表现为hpre不指向NULL)，那hnow就是处在链表中间位置。
        if (hpre != NULL) {
            hpre->next = hnow->next;
            hnow->next = VocabHash[hval];
            VocabHash[hval] = hnow;
        }
    }
    hnow = NULL;
    hpre = NULL;
}

int GetWord(FILE *CorpusFile, char *Word) {
    int i = 0,ch;
    for (;;) {
        ch = fgetc(CorpusFile);
        if (ch == '\r') continue;
        if (i == 0 && ((ch == '\n') || (ch ==EOF) || (ch == '\0') || (ch == '\v') || (ch =='\f'))) {
            Word[i] = 0;
            return 1;
        }
        if (i == 0 && ((ch == ' ') || (ch == '\t'))) {
            continue;
        }
        if (i != 0 && ((ch == EOF) || (ch == ' ') || (ch == '\t') || (ch == '\n') || (ch == '\0') || (ch == '\v') || (ch =='\f'))) {
            if (ch == '\n') {
                ungetc(ch, CorpusFile);
            }
            break;
        }
        if (i < MaxWordLen) {
            // 处理标点符号
            if ((ispunct(ch) || ch == '\'') && i!=0){
                ungetc(ch, CorpusFile);
                Word[i] = 0;
                return 0;
            }
            if ((ispunct(ch)) && i == 0) {
                Word[i++] = ch;
                Word[i] = 0;
                return 0;
            }
            // 大写改成小写
            if (ch > 64 && ch < 91) {
                ch += 32;
            }
            Word[i++] = ch;
        }
    }
    Word[i] = 0;
    // avoid truncation destroying a multibyte UTF-8 char except if only thing on line (so the i > x tests won't overwrite word[0])
    // see https://en.wikipedia.org/wiki/UTF-8#Description
    if (i == MaxWordLen - 1 && (Word[i-1] & 0x80) == 0x80) {
        if ((Word[i-1] & 0xC0) == 0xC0) {
            Word[i-1] = '\0';
        } else if (i > 2 && (Word[i-2] & 0xE0) == 0xE0) {
            Word[i-2] = '\0';
        } else if (i > 3 && (Word[i-3] & 0xF8) == 0xF0) {
            Word[i-3] = '\0';
        }
    }
    return 0;
}

int CompareVocab(const void *a, const void *b) {
    long long c;
    if ( (c = ((ARRAYUNIT *)b)->Count - ((ARRAYUNIT *)a)->Count) != 0 ) {
        return c > 0 ? 1 : -1;
    } else {
        return 0;
    }
}

int CompareVocabTie(const void *a, const void *b) {
    long long c;
    if ( (c = ((ARRAYUNIT *) b)->Count - ((ARRAYUNIT *) a)->Count) != 0) return ( c > 0 ? 1 : -1 );
    else return (scmp(((ARRAYUNIT *) a)->Word,((ARRAYUNIT *) b)->Word));
}

long long HashToArray(HASHUNITID **VocabHash, ARRAYUNIT *VocabArray, long long VocabSize) {
    // 变量定义区，第一次阅读可以跳过，
    // 后面用到再反过来看
    HASHUNITID *htmp= NULL;
    long long ArrayCounter=0;
    // 开始遍历VocabHash的每一行以及每一行中的所有链表
    for (long long HashRow=0;HashRow < TSIZE;HashRow++) {
        htmp = VocabHash[HashRow];
        // 只要htmp不为空，就需要把这一行的所有链表并入数组中
        // 因为这里只是字符串指针的赋值，不需要再开辟内存并拷贝，所以很快
        while (htmp != NULL) {
            VocabArray[ArrayCounter].Word = htmp->Word;
            VocabArray[ArrayCounter].Count = htmp->Count;
            ArrayCounter++;
            // 如果空间不够了，还得再开辟新的内存
            if (ArrayCounter >= VocabSize) {
                VocabSize += 2500;
                VocabArray = (ARRAYUNIT *) realloc(VocabArray, sizeof(ARRAYUNIT) * VocabSize);
            }
            htmp = htmp->next;
        }
    }
    // 这里似乎是不需要再赋一次空指针，
    // 因为当初构造链表的时候保证了链表末尾一定是空指针。
    // Anyway，再写一次也无妨。
    htmp = NULL;
    return ArrayCounter;
}

void CutVocab(ARRAYUNIT *VocabArray, long long MaxVocab, long long MinCount, long long VocabSize, int IfSaveVocab) {
    // 先砍掉超出最大词汇表长度的单词
//    fprintf(stderr, "\nMaxVocab: %lld; arrarlen: %lld\n", MaxVocab, VocabSize);
    if (MaxVocab > 0 && MaxVocab < VocabSize)
        qsort(VocabArray, VocabSize, sizeof(ARRAYUNIT), CompareVocab);
    else MaxVocab = VocabSize;
    // CompareVocabTie 按照单词首字母的大小裁定两个词频一样的单词谁先谁后
    qsort(VocabArray, MaxVocab, sizeof(ARRAYUNIT), CompareVocabTie);
    // 从临界词频（刚刚比MinCount大的词频）处截断词表
    for (long long FinalVocabSize=0; FinalVocabSize < MaxVocab; FinalVocabSize++) {
        if (VocabArray[FinalVocabSize].Count < MinCount) {
            // 将词频不够的单词对应的词字符串指针设为空，count清零。
            VocabArray[FinalVocabSize].Word = NULL;
            VocabArray[FinalVocabSize].Count = 0;
        } else if (IfSaveVocab) {
            // 输出到屏幕或者在终端用管道命令行输出到txt文档。
            printf("%s %lld\n", VocabArray[FinalVocabSize].Word, VocabArray[FinalVocabSize].Count);
        }
    }
    // 将超过最大词汇表长度部分的词串指针设为空，count清零。
    if (MaxVocab < VocabSize) {
        for (long long FinalVocabSize=MaxVocab; FinalVocabSize < VocabSize; FinalVocabSize++) {
            VocabArray[FinalVocabSize].Word = NULL;
            VocabArray[FinalVocabSize].Count = 0;
        }
    }
}

void ArrayToHashWithID(ARRAYUNIT *VocabArray, HASHUNITID **NewVocabHash){
    // 将词汇按词频从大到小的顺序编号并存入哈希表中，从0开始编号。
    // 计算当前单词的hash value，这里不采用传入词汇表长度，
    // 而是另外重新计数是为了让这个函数用起来更加self-contained。更加方便。
    long long VocabSize = 0;
    unsigned int hval;
    HASHUNITID *hpre= NULL, *hnow= NULL;
    while (VocabArray[VocabSize].Word != NULL){
        VocabSize += 1;
    }
    for (long long i=0; i < VocabSize; i++) {
        hval = HashValue(VocabArray[i].Word, TSIZE, SEED);
        // 这里不调用HashMapWord,因为我们不需要重新申请内存，但是代码就会很像
        for (   hpre = NULL,hnow = NewVocabHash[hval];
                hnow != NULL && scmp(hnow->Word, VocabArray[i].Word)!=0;
                hpre = hnow, hnow = hnow->next );
        if (hnow == NULL) {
            // hnow为空表明该指针没有指向任何内存，所以需要先开辟内存
            // 然后把hnow指向这个内存。
            hnow = (HASHUNITID *) malloc(sizeof(HASHUNITID));
            hnow->Word = VocabArray[i].Word;
            hnow->Count = VocabArray[i].Count;
            hnow->id = i;
            hnow->next = NULL;
            if (hpre == NULL) {
                // hnow指向的那块内存是VocabHash[hval]处的第一个节点
                NewVocabHash[hval] = hnow;
            } else {
                // 将hnow接到hpre后面
                hpre->next = hnow;
            }
        } else {
            // hnow不为空，就说明遇到同一个词了
            // 将该词Count+1，同时将hnow指向的内存挂在链表的第一个节点
            hnow->Count++;
            // 先判断hnow所指向的内存在链表中是不是已经处在第一个节点了？
            // 如果是的话(表现为hpre指向NULL)，那就不用再移动了;
            // 如果不是的话(表现为hpre不指向NULL)，那hnow就是处在链表中间位置。
            if (hpre != NULL) {
                hpre->next = hnow->next;
                hnow->next = NewVocabHash[hval];
                NewVocabHash[hval] = hnow;
            }
        }
        hnow = NULL;
        hpre = NULL;
    }
}

void FillIdToVocabHash(ARRAYUNIT *VocabArray, HASHUNITID **VocabHash) {
    // 遍历VocabArray，查找其中的每个单词在VocabHash中的位置
    // 将词汇按词频从大到小的顺序编号并存入哈希表中，从0开始编号。
    // 计算当前单词的hash value，这里不采用传入词汇表长度，
    // 而是另外重新计数是为了让这个函数用起来更加self-contained。更加方便。
    long long VocabSize = 0;
    unsigned int hval;
    HASHUNITID *hnow= NULL;
    while (VocabArray[VocabSize].Word != NULL){
        VocabSize += 1;
    }
    for (long long i=0; i < VocabSize; i++) {
        hval = HashValue(VocabArray[i].Word, TSIZE, SEED);
        hnow = VocabHash[hval];
        while (hnow != NULL) {
            if (scmp(hnow->Word,VocabArray[i].Word) == 0) {
                hnow->id = i;
                break;
            } else {
                hnow = hnow->next;
            }
        }
    }
    hnow = NULL;
}

long long HashSearch(char *Word, HASHUNITID **NewVocabHash) {
    // 如果找不到就返回-1
    long long id = -1;
    unsigned int hval = HashValue(Word,TSIZE,SEED);
    HASHUNITID *htmp = NULL;
    htmp = NewVocabHash[hval];
    while (htmp != NULL) {
        if (htmp->Word != NULL && scmp(htmp->Word,Word) == 0) {
            id = htmp->id;
            break;
        } else {
            htmp = htmp->next;
        }
    }
    return id;
}

ARRAYUNIT * BuildVocab(FILE *CorpusFile, HASHUNITID **VocabHash,long long MaxVocab, long long MinCount, int IfSaveVocab, int IfSaveCoocur) {
    // 变量定义区，第一次阅读可以跳过，
    // 后面用到再反过来看
    int IfNotGet=1;
    char Word[MaxWordLen];
    long long TokenCounter=0;
    long long VocabSize = 1717500;
    ARRAYUNIT *VocabArray = (ARRAYUNIT *)malloc(sizeof(ARRAYUNIT) * VocabSize);
    // 检查是否能够打开语料
    fprintf(stderr, "Building Vocabulary.");
    // 逐个读入CorpusFile中的每个字符，
    // 并将它们初步压入一个哈希表中，
    // 哈希冲突则将单词使用链表挂在后面。
    while (!feof(CorpusFile)) {
        IfNotGet = GetWord(CorpusFile, Word);
        if (IfNotGet) { continue;}
        HashMapWord(Word,VocabHash);
        if (((++TokenCounter) % 100000) == 0) {
            fprintf(stderr, "\rHave read %lld tokens so far.",TokenCounter);
        }
    }
    // 将带链表的哈希表转化为数组array，方便后面排序
    VocabSize = HashToArray(VocabHash,VocabArray,VocabSize);
    fprintf(stderr, "\nCounted %lld unique words.\n", VocabSize);
    // 开始利用最大词表长度以及最小词频限制删减词典,并将词表保存在一个文件中
    CutVocab(VocabArray,MaxVocab,MinCount,VocabSize, IfSaveVocab);
    // 根据VocabArray再给哈希表里每个单词赋予id：
    FillIdToVocabHash(VocabArray,VocabHash);
    // 释放内存，防止出现野指针与内存泄漏。
    // 当后面还需要保存共现矩阵时，不执行此操作。
    if (IfSaveCoocur == 0) {
        HASHUNITID *htmp= NULL,*hpre= NULL;
        for (int i=0;i<TSIZE;i++) {
            if (VocabHash[i] != NULL) {
                htmp = VocabHash[i];
                while (htmp != NULL) {
                    free(htmp->Word);
                    hpre = htmp;
                    htmp = htmp->next;
                    free(hpre);
                    hpre = NULL;//防止hpre变成野指针，随机指向某个内存区域
                }
            }
        }
        free(VocabArray);
        free(VocabHash);
        htmp = NULL;
        hpre = NULL;
    } else { // 当后面还需要保存共现矩阵时，把被砍掉的词汇（表现为id=-1）对应的字符串释放掉，但是指针本身还得保存，方便后续哈希查找。
        HASHUNITID *htmp= NULL;
        for (int i=0;i<TSIZE;i++) {
            if (VocabHash[i] != NULL) {
                htmp = VocabHash[i];
                while (htmp != NULL) {
                    if (htmp->id == -1) {
                        free(htmp->Word);
                        htmp->Word = NULL;
                        htmp->Count = 0;
                    }
                    htmp = htmp->next;
                }
            }
        }
        htmp = NULL;
    }
    return VocabArray;
}

void GetLine(std::vector<long long> *line, FILE *CorpusFile, HASHUNITID **NewVocabHash) {
    int IfNotGet=0;
    char Word[MaxWordLen];
    long long id;
    // 清空vector
    line->clear();
    // 开始循环读词直到IfNotGet变为1,表示没有读到任何词，换行了
    while (IfNotGet == 0 && feof(CorpusFile)==0) {
        IfNotGet = GetWord(CorpusFile,Word);
        if (IfNotGet == 0) {
            id = HashSearch(Word,NewVocabHash); // 如果没找到，id就会为-1
            if (id != -1) {
                line->push_back(id);
            }
        }
    }
}

int BuildCoocur(FILE *CorpusFile, ARRAYUNIT *VocabArray, HASHUNITID **NewHashVocab,char *CoocurOutputFile,int HalfWinWidth, int IfSaveCoocur) {
    fprintf(stderr,"Building Coocurance Matrix\n");
    // 变量定义区，第一次阅读可以跳过，
    // 后面用到再反过来看
    FILE *fCoocurOut;
    std::vector<long long> line;
    long long VocabSize = 0, *lookup= NULL, lookupIndex, InWordId, OutWordId, InWordRowNum;
    int *CoocurMatrix = NULL, lineCounter=0;
    while (VocabArray[VocabSize].Word != NULL){
        VocabSize += 1;
    }
    // Build auxiliary lookup table used to index into the Coocur matrix.
    lookup = (long long *) calloc(VocabSize+1, sizeof(long long));
    if (lookup == NULL) {
        fprintf(stderr, "Couldn't allocate memory!");
        return 1;
    }
    lookup[0] = 0;
    for (lookupIndex=1; lookupIndex <= VocabSize; lookupIndex++) {
        // 这里所有下标都是从0开始计数
        // 可以举一个只有3个单词的例子验证一下加减的常数
        lookup[lookupIndex] = lookup[lookupIndex-1] + VocabSize + 1;
    }
    // allocate memory for full coocur matrix
    CoocurMatrix = (int *)calloc(lookup[lookupIndex-1]+1, sizeof(int));
    if (CoocurMatrix == NULL) {
        fprintf(stderr, "\nCouldn't allocate memory!");
        return 1;
    }
    fprintf(stderr, "Coocur matrix contains %lld elements theoraticaly\n",lookup[lookupIndex-1]);
    // 开始统计共现信息。先将CorpusFile文件的指针置放在文件开头。
    rewind(CorpusFile);
    // 首先摘取一整段的文本，将字符全查哈希表后变成id组成“line” vector。
    fCoocurOut = fopen(CoocurOutputFile,"w+");
    while (!feof(CorpusFile)) {
        GetLine(&line,CorpusFile,NewHashVocab);
        // 开始严格按照fasttext的风格对共现矩阵采样。
        for (int w=0; w < line.size(); w++) { //最外层循环中心词。
            InWordId = line[w];
            // 查询当前InWordId所在CoocurMatrix矩阵的行号
            InWordRowNum = lookup[InWordId];
            for (int c = -HalfWinWidth; c <= HalfWinWidth; c++) {
                if (c!=0 && w + c >= 0 && w + c < line.size()) {
                    OutWordId = line[w+c];
                    CoocurMatrix[InWordRowNum + OutWordId] += 1;
                }
            }
        }
        lineCounter += 1;
        if (lineCounter%10000 == 0) {
            fprintf(stderr, "\rHave processed %d lines.", lineCounter);
        }
    }
    fprintf(stderr, "\n");
    if (IfSaveCoocur==1) {
        // 开始保存共现矩阵,利用了VocabArray
        for (long long i=0; i < lookup[lookupIndex-1]; i++) {
            int a = 0;
            if (CoocurMatrix[i]==0) { continue;}
            InWordId = i / (VocabSize + 1);
            OutWordId = i % (VocabSize + 1);
            fprintf(fCoocurOut, "%s\t%s\t%d\n",VocabArray[InWordId].Word, VocabArray[OutWordId].Word, CoocurMatrix[i]);
            if (i % 10000 == 0) {
                fprintf(stderr, "\rHave saved %lld pairs.",i);
            }
        }
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "Saving cooccurrences done.\n");
    fprintf(stderr, "Freeing the heap and stack memory!\n");
    fclose(fCoocurOut);
    free(lookup);
    free(CoocurMatrix);
    HASHUNITID *htmp= NULL,*hpre= NULL;
    for (int i=0;i<TSIZE;i++) {
        if (NewHashVocab[i] != NULL) {
            htmp = NewHashVocab[i];
            while (htmp != NULL) {
                free(htmp->Word);
                hpre = htmp;
                htmp = htmp->next;
                free(hpre);
                hpre = NULL;//防止hpre变成野指针，随机指向某个内存区域
            }
        }
    }
    free(VocabArray);
    free(NewHashVocab);
    htmp = NULL;
    hpre = NULL;
    fprintf(stderr, "Heap and stack memory free done!\n");
    return 0;
}

int find_flag(char *str, int argc, char **argv) {
    int i;
    // 逐步循环比较str字符串是否在argv二维字符数组中出现过,
    // i跳过0是因为argv的第一个字符串是“./Glove”本身，不需要比较
    for (i = 1; i < argc; i++) {
        // 逐个比较argv中的每个字符串,
        // 如果找到了，那就把当前的位置传回去,
        // 否则继续循环直到跑到了argv数组末尾
        if (scmp(str,argv[i])) {
            // 当循环到了最后一位时，
            // 需要反馈哪一个flag没有传入argv中
            if (i == argc - 1){
                fprintf(stderr,"You didn't pass in the value of flag:%s\n", str);
                return 0;
            }
            continue;
        } else {
            return i;
        }
    }
    return -1;
}

int main(int argc, char **argv) {
    // 变量定义区，第一次阅读可以跳过，
    // 后面用到再反过来看
    int i,verbose=2, HalfWinWidth=5;
    long long MaxVocab=12500,MinCount=0;//12000词汇量基本等同于流行报纸每天文章里的词汇量。
    int IfBuildVocab=1,IfSaveVocab=1;
    int IfBuildCoocur=1,IfSaveCoocur=1;
    char CoocurOutputFile[200];

    // 读取flag
    if ( (i = find_flag((char*)"--help", argc, argv)) > 0 ) {
        // 打印帮助信息，不用看。
        printf("Simple tool to build vocab and cooccurrence\n");
        printf("mhyao (mhyao@mail.ustc.edu.cn)\n\n");
        printf("Usage options:\n");
        printf("\t-verbose <int>\n");
        printf("\t\tSet verbosity: 0, 1, or 2 (default)\n");
        printf("\t-IfBuildVocab <int>\n");
        printf("\t\t0 for not save (default), 1 for save\n");
        printf("\t-IfBuildCoocur <int>\n");
        printf("\t\t0 for not save (default), 1 for save\n");
        printf("\t-CoocurOutputFile <str>\n");
        printf("\t\tThe output file name for saving the coocur matrix\n");
        printf("\t-IfSaveVocab <int>\n");
        printf("\t\t0 for not save (default), 1 for save\n");
        printf("\t-IfSaveCoocur <int>\n");
        printf("\t\t0 for not save (default), 1 for save\n");
        printf("\t-max-vocab <int>\n");
        printf("\t\tUpper bound on vocabulary size (default 12500), i.e. keep the <int> most frequent words. The minimum frequency words are randomly sampled so as to obtain an even distribution over the alphabet.\n");
        printf("\t-min-count <int>\n");
        printf("\t\tLower limit such that words which occur fewer than <int> times are discarded.\n");
        printf("\t-HalfWinWidth <int>\n");
        printf("\t\tThe half window width with default value being 5.\n");
        printf("\nExample usage:\n");
        printf("./BuildVocabAndCooccur -verbose 2 -IfBuildVocab 1 -IfSaveVocab 1 -IfBuildCoocur 1 -IfSaveCoocur 1 -max-vocab 50000 -min-count 10 -CoocurOutputFile cooccur.txt -HalfWinWidth 5 < corpus.txt > vocab.txt\n");
        return 0;
    }
    if ( (i = find_flag((char*)"-verbose", argc, argv)) > 0 ) {
        verbose = atoi(argv[i+1]);
        fprintf(stderr, "The verbosity is: %d\n", verbose);
    }
    if ( (i = find_flag((char*)"-IfBuildVocab", argc, argv)) > 0 ) {
        IfBuildVocab = atoi(argv[i+1]);
        fprintf(stderr, "If build the vocab?: %d\n", IfBuildVocab);
    }
    if ( (i = find_flag((char*)"-IfSaveVocab", argc, argv)) > 0 ) {
        IfSaveVocab = atoi(argv[i+1]);
        fprintf(stderr, "If save the vocab?: %d\n", IfSaveVocab);
    }
    if ( (i = find_flag((char*)"-IfBuildCoocur", argc, argv)) > 0 ) {
        IfBuildCoocur = atoi(argv[i+1]);
        fprintf(stderr, "If build the coocur matrix?: %d\n", IfBuildCoocur);
    }
    if ( (i = find_flag((char*)"-CoocurOutputFile", argc, argv)) > 0 ) {
        strcpy(CoocurOutputFile, argv[i+1]);
        fprintf(stderr, "Coocur matrix output file name: %s\n", CoocurOutputFile);
    }
    if ( (i = find_flag((char*)"-IfSaveCoocur", argc, argv)) > 0 ) {
        IfSaveCoocur = atoi(argv[i+1]);
        fprintf(stderr, "If save the coocur matrix?: %d\n", IfSaveCoocur);
    }
    if ( (i = find_flag((char*)"-max-vocab", argc, argv)) > 0 ) {
        MaxVocab = atoll(argv[i+1]);
        fprintf(stderr, "The max vocab size: %lld\n", MaxVocab);
    }
    if ( (i = find_flag((char*)"-HalfWinWidth", argc, argv)) > 0 ) {
        HalfWinWidth = atoi(argv[i+1]);
        fprintf(stderr, "The half of window width: %d\n", HalfWinWidth);
    }
    if ( (i = find_flag((char*)"-min-count", argc, argv)) > 0 ) {
        MinCount = atoll(argv[i+1]);
        fprintf(stderr, "The min vocab count: %lld\n", MinCount);
    }
    // 建立词汇表，并以txt形式保存词汇表和对应词频
    if (IfBuildVocab) {
        // 从命令行中读取语料
        FILE *CorpusFile = stdin;
        HASHUNITID **VocabHash = InitHashTableID(TSIZE);
        ARRAYUNIT *VocabArray = BuildVocab(CorpusFile, VocabHash,MaxVocab, MinCount, IfSaveVocab, IfSaveCoocur);
        // 统计共现矩阵，并以txt形式保存(word1, word2, CoocurTimes)三元组
        if (IfBuildCoocur) {
            BuildCoocur(CorpusFile,VocabArray,VocabHash,CoocurOutputFile,HalfWinWidth,IfSaveCoocur);
        }
    }
    return 0;
}