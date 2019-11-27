//
// Created by hutao on 19-11-26.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#define TSIZE 1048576
#define SEED  1159241
#define MaxWordLen 1000
#define HASHFN  HashValue

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
    // 两个字符串不相同时，返回1；
    // 相同时，返回0。
    while (*s1 != '\0' && *s1 == *s2) {s1++;s2++;}
    return *s1 - *s2;
}

HASHUNIT ** InitHashTable() {
    HASHUNIT **ht = (HASHUNIT **) malloc(sizeof(HASHUNIT *) * TSIZE);
    for (int i=0;i<TSIZE;i++) {
        ht[i] = (HASHUNIT *) NULL;
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

void HashMapWord(char *Word, HASHUNIT **VocabHash) {
    HASHUNIT *hpre, *hnow;
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
        hnow = (HASHUNIT *) malloc(sizeof(HASHUNIT));
        hnow->Word = (char *) malloc(strlen(Word) + 1);
        strcpy(hnow->Word, Word);
        hnow->Count = 1;
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
    return;
}

int GetWord(FILE *CorpusFile, char *Word) {
    int i = 0, ch;
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
            if (ispunct(ch) && i!=0){
                ungetc(ch, CorpusFile);
                Word[i] = 0;
                return 0;
            }
            if (ispunct(ch) && i == 0) {
                Word[i++] = ch;
                Word[i] = 0;
                return 0;
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

long long HashToArray(HASHUNIT **VocabHash, ARRAYUNIT *VocabArray, long long VocabSize) {
    // 变量定义区，第一次阅读可以跳过，
    // 后面用到再反过来看
    HASHUNIT *htmp;
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
    return ArrayCounter;
}

void CutVocab(ARRAYUNIT *VocabArray, long long MaxVocab, long long MinCount, long long VocabSize, int IfSaveVocab) {
    // 先砍掉超出最大词汇表长度的单词
    fprintf(stderr, "\nMaxVocab: %lld; arrarlen: %lld\n", MaxVocab, VocabSize);
    if (MaxVocab > 0 && MaxVocab < VocabSize)
        qsort(VocabArray, VocabSize, sizeof(ARRAYUNIT), CompareVocab);
    else MaxVocab = VocabSize;
    // CompareVocabTie 按照单词首字母的大小裁定两个词频一样的单词谁先谁后
    qsort(VocabArray, MaxVocab, sizeof(ARRAYUNIT), CompareVocabTie);
    // 从临界词频（刚刚比MinCount大的词频）处截断词表
    for (int FinalVocabSize=0; FinalVocabSize < MaxVocab; FinalVocabSize++) {
        if (VocabArray[FinalVocabSize].Count < MinCount) {
            break;
        }
        // 输出到屏幕或者在终端用管道命令行输出到txt文档。
        printf("%s %lld\n", VocabArray[FinalVocabSize].Word, VocabArray[FinalVocabSize].Count);
    }
}

ARRAYUNIT * BuildVocab(FILE *CorpusFile, long long MaxVocab, long long MinCount, int IfSaveVocab) {
    // 变量定义区，第一次阅读可以跳过，
    // 后面用到再反过来看
    int IfNotGet=1;
    char Word[MaxWordLen];
    long long TokenCounter=0;
    long long VocabSize = 1717500;
    HASHUNIT **VocabHash = InitHashTable();
    ARRAYUNIT *VocabArray = (ARRAYUNIT *)malloc(sizeof(ARRAYUNIT) * VocabSize);
    // 检查是否能够打开语料

    // 逐个读入CorpusFile中的每个字符，
    // 并将它们初步压入一个哈希表中，
    // 哈希冲突则将单词使用链表挂在后面。
    while (!feof(CorpusFile)) {
        IfNotGet = GetWord(CorpusFile, Word);
        if (IfNotGet) { continue;}
        HashMapWord(Word,VocabHash);
        if (((++TokenCounter) % 100000) == 0) {
            fprintf(stderr, "\rHave read\033[11G %lld tokens so far.",TokenCounter);
        }
    }
    // 将带链表的哈希表转化为数组array，方便后面排序
    VocabSize = HashToArray(VocabHash,VocabArray,VocabSize);
    fprintf(stderr, "\nCounted %lld unique words.\n", VocabSize);
    // 开始利用最大词表长度以及最小词频限制删减词典,
    CutVocab(VocabArray,MaxVocab,MinCount,VocabSize, IfSaveVocab);
    // 释放内存，防止出现野指针
    HASHUNIT *htmp,*hpre;
    for (int i=0;i<TSIZE;i++) {
        if (VocabHash[i] != NULL) {
            htmp = VocabHash[i];
            while (htmp != NULL) {
                free(htmp->Word);
                hpre = htmp;
                htmp = htmp->next;
                free(hpre);
            }
        }
    }
    free(VocabArray);
    free(VocabHash);
    return VocabArray;
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
                printf("You didn't pass in the value of flag:%s\n", str);
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
    int i,verbose=2;
    long long MaxVocab=12500,MinCount=0;//12000词汇量基本等同于流行报纸每天文章里的词汇量。
    int IfBuildVocab=0,IfSaveVocab=0;
    int IfBuildCoocur=0,IfSaveCoocur=0;
    // 打印帮助信息，不用看。
    if (argc == 1) {
        printf("Simple tool to extract unigram counts\n");
        printf("Author: Jeffrey Pennington (jpennin@stanford.edu)\n\n");
        printf("Usage options:\n");
        printf("\t-verbose <int>\n");
        printf("\t\tSet verbosity: 0, 1, or 2 (default)\n");
        printf("\t-IfBuildVocab <int>\n");
        printf("\t\t0 for not save (default), 1 for save\n");
        printf("\t-IfBuildCoocur <int>\n");
        printf("\t\t0 for not save (default), 1 for save\n");
        printf("\t-IfSaveVocab <int>\n");
        printf("\t\t0 for not save (default), 1 for save\n");
        printf("\t-IfSaveCoocur <int>\n");
        printf("\t\t0 for not save (default), 1 for save\n");
        printf("\t-max-vocab <int>\n");
        printf("\t\tUpper bound on vocabulary size (default 12500), i.e. keep the <int> most frequent words. The minimum frequency words are randomly sampled so as to obtain an even distribution over the alphabet.\n");
        printf("\t-min-count <int>\n");
        printf("\t\tLower limit such that words which occur fewer than <int> times are discarded.\n");
        printf("\nExample usage:\n");
        printf("./vocab_count -verbose 2 -IfBuildVocab 1 -IfSaveVocab 1 -IfBuildCoocur 1 -IfSaveCoocur 1 -max-vocab 100000 -min-count 10 < corpus.txt > vocab.txt\n");
        return 0;
    }
    // 读取flag
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
        fprintf(stderr, "The output vocab file would be [CorpusFile]+[Vocab].txt\n");
    }
    if ( (i = find_flag((char*)"-IfBuildCoocur", argc, argv)) > 0 ) {
        IfBuildCoocur = atoi(argv[i+1]);
        fprintf(stderr, "If build the coocur matrix?: %d\n", IfBuildCoocur);
    }
    if ( (i = find_flag((char*)"-IfSaveCoocur", argc, argv)) > 0 ) {
        IfSaveVocab = atoi(argv[i+1]);
        fprintf(stderr, "If save the coocur matrix?: %d\n", IfSaveVocab);
        fprintf(stderr, "The output coocur matrix file would be [CorpusFile]+[Coocur].txt\n");
    }
    if ( (i = find_flag((char*)"-max-vocab", argc, argv)) > 0 ) {
        MaxVocab = atoll(argv[i+1]);
        fprintf(stderr, "The max vocab size: %lld\n", MaxVocab);
    }
    if ( (i = find_flag((char*)"-min-count", argc, argv)) > 0 ) {
        MinCount = atoll(argv[i+1]);
        fprintf(stderr, "The min vocab count: %lld\n", MinCount);
    }
    // 建立词汇表，并以txt形式保存词汇表和对应词频
    if (IfBuildVocab) {
        // 从命令行中读取语料
        FILE *CorpusFile = stdin;
        ARRAYUNIT * VocabArray = BuildVocab(CorpusFile, MaxVocab, MinCount, IfSaveVocab);
    }
    // 统计共现矩阵，并以txt形式保存(word1, word2, CoocurTimes)三元组
    if (IfBuildCoocur) {
        fprintf(stderr, "ToDo");
    }
    return 0;
}
