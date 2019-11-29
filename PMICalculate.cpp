//
// Created by mhyao on 19-11-28.
//
#include <fstream>
#include <iostream>
#include <cstring>
#include <stdlib.h>
#define TSIZE 1048576
#define SEED  1159241
#define HASHFN  HashValue
int MaxWordLen = 100;

typedef struct line {
    char InWord[100];
    char OutWord[100];
    char Count[100];
} LINE;

typedef struct VocabHashWithId {
    char *Word;
    char *freq;
    long long id;
    struct VocabHashWithId *next;
} HASHUNITID;

HASHUNITID ** InitHashTableID(int Tsize) {
    HASHUNITID **ht = (HASHUNITID **) malloc(sizeof(HASHUNITID *) * Tsize);
    for (int i=0;i<Tsize;i++) {
        ht[i] = (HASHUNITID *) NULL;
    }
    return ht;
}

int scmp(char *s1, char *s2) {
    // 以s1字符串为标准，看s2是不是等于s1
    // 两个字符串不相同时，返回1；
    // 相同时，返回0。
    while (*s1 != '\0' && *s1 == *s2) {s1++;s2++;}
    return *s1 - *s2;
}

unsigned int HashValue(char *word, int tsize, unsigned int seed) {
    char c;
    unsigned int h;
    h = seed;
    for ( ; (c = *word) != '\0'; word++) h ^= ((h << 5) + c + (h >> 2));
    return (unsigned int)((h & 0x7fffffff) % tsize);
}

void HashMapWord(char *Word, char*freq, HASHUNITID **VocabHash) {
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
        hnow->freq = (char *) malloc(strlen(freq) + 1);
        strcpy(hnow->Word, Word);
        strcpy(hnow->freq, freq);
        hnow->id = -1;
        hnow->next = NULL;
        if (hpre == NULL) {
            // hnow指向的那块内存是VocabHash[hval]处的第一个节点
            VocabHash[hval] = hnow;
        } else {
            // 将hnow接到hpre后面
            hpre->next = hnow;
        }
    }
    hnow = NULL;
    hpre = NULL;
}

float HashSearch(char *Word, HASHUNITID **VocabHash) {
    // 如果找不到就返回-1
    float freq = 0.0;
    unsigned int hval = HashValue(Word,TSIZE,SEED);
    HASHUNITID *htmp = NULL;
    htmp = VocabHash[hval];
    while (htmp != NULL) {
        if (htmp->Word != NULL && scmp(htmp->Word,Word) == 0) {
            freq = atof(htmp->freq);
            break;
        } else {
            htmp = htmp->next;
        }
    }
    return freq;
}

int GetLines(FILE *fvocab,LINE *Line) {
    int ch, term=0,letter=0;//term表示该行有几个词：2个词的话，就是一个w_i,一个Count_i;3个词的话就是一个w_i,一个w_j，一个Count_ij
    char line[3][MaxWordLen];
    while (1) {
        ch = fgetc(fvocab);
        // 将大写转化为小写
        if (ch > 64 && ch < 91) {
            ch += 32;
        }
        line[term][letter++] = ch;
        // 遇到tab键就表示读完了一个term，增加一次计数，并换line的下一个字符串
        if (ch == '\t' || ch == ' '|| ch == '\n' || ch == EOF) {letter--;line[term][letter]='\0';};
        if (ch == '\t' || ch == ' ') {term +=1;letter=0;};
        // 遇到转行就退出循环
        if (ch == '\n' || ch ==EOF) {
            break;
        }
    }
    if (term == 1) {
        strcpy(Line->InWord,line[0]);
        strcpy(Line->Count,line[1]);
        if (ch == EOF) {
            return 1;
        } else { return 0;}
    } else { // term == 2
        strcpy(Line->InWord,line[0]);
        strcpy(Line->OutWord,line[1]);
        strcpy(Line->Count,line[2]);
        if (ch == EOF) {
            return 1;
        } else { return 0;}
    }
}

//./PMICalculater -VocabFile vocab.txt, -CoocurFile cooccur.txt, -WordFreqOutputFile wordfreq.txt, -ConditionProbFile condiprob.txt, -PMIFile pmi.txt
// -VocabFile tempTestVocab.txt -CoocurFile src/tempTestCoocur.txt -WordFreqOutputFile tempTestfreq.txt -ConditionProbFile tempTestCondiProb.txt -PMIFile tempTestPMI.txt

void CalculatePMI(char*ConditionProbFile,char*WordFreqOutputFile,char*PMIFile) {
    fprintf(stderr,"Building pmi file.\n");
    FILE *ffreq = fopen(WordFreqOutputFile,"r");
    FILE *fcondpro = fopen(ConditionProbFile,"r");
    FILE *fpmi = fopen(PMIFile,"w+");
    LINE *line = (LINE *) malloc(sizeof(LINE));
    HASHUNITID **VocabHash = InitHashTableID(TSIZE);
    char InWord[100];
    GetLines(fcondpro,line);
    strcpy(InWord,line->InWord);
    fprintf(stderr,"Processing: %s\n",InWord);
    rewind(fcondpro);
    // 首先建立词频表
    while (!feof(ffreq)) {
        GetLines(ffreq,line);
        HashMapWord(line->InWord,line->Count,VocabHash);
    }
    // 逐行遍历条件概率文件，计算pmi
    while  (!feof(fcondpro)) {
        GetLines(fcondpro,line);
        fprintf(fpmi,"%s\t%s\t%lf\n", line->InWord,line->OutWord,(atof(line->Count)/HashSearch(line->OutWord,VocabHash)));
        if (scmp(InWord,line->InWord) != 0) {
            strcpy(InWord,line->InWord);
            fprintf(stderr,"Processing: %s\n",InWord);
        }
    }
    fclose(ffreq);
    fclose(fcondpro);
    fclose(fpmi);
    // 释放内存
    fprintf(stderr,"freeing the memory.\n");
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
    free(VocabHash);
    htmp = NULL;
    hpre = NULL;
    fprintf(stderr,"memory free done.\n");
}

void ConditionalProba(char*CoocurFile, char*ConditionProbFile) {
    fprintf(stderr,"Building conditional probability file.\n");
    FILE *fcooccur = fopen(CoocurFile,"r");
    FILE *fsave = fopen(ConditionProbFile,"w+");
    LINE *Line= (LINE*) malloc(sizeof(LINE));
    char InWord[100];
    GetLines(fcooccur,Line);
    strcpy(InWord,Line->InWord);
    fprintf(stderr,"Processing: %s\n",InWord);
    rewind(fcooccur);
    int end = 0;
    float MarginalCount=0.0;
    fpos_t ParaBegin,LineBegin;
    fgetpos(fcooccur,&ParaBegin);
    while (1) {
        end = GetLines(fcooccur,Line);
        if (end == 1) { break;}
        // 判断当前读到的InWord是否发生了改变
        // 没有发生改变那就累加计数，并重定位到本段的开头，重新计算所有的条件概率
        // 所以我们需要记录本端开头的位置
        if (scmp(InWord,Line->InWord) == 0) {
            // 依然保持同一个词，所以累计
            MarginalCount += atoi(Line->Count);
        } else {// 回到本段的开始，重新计算条件概率
            fsetpos(fcooccur,&ParaBegin);
            // 循环本段
            while (1) {
                // 逐个写入save文件
                // 并计算概率
                // 定位每一行的位置
                fgetpos(fcooccur,&LineBegin);
                GetLines(fcooccur,Line);
                if (scmp(InWord,Line->InWord)==0){
                    fprintf(fsave,"%s\t%s\t%lf\n",Line->InWord,Line->OutWord,atoi(Line->Count)/MarginalCount);
                } else {
                    break;
                }
            }
            // 回到下一段的开头，也就是回退一行
            ParaBegin = LineBegin;
            fsetpos(fcooccur,&ParaBegin);
            MarginalCount = 0.0;// 重置累计数
            strcpy(InWord,Line->InWord);// 更新InWord记载
            fprintf(stderr,"Processing: %s\n",InWord);
        }
    }
    fclose(fcooccur);
    fclose(fsave);
}

void WordFreq(char*VocabFile, char*WordFreqOutputFile){
    fprintf(stderr,"Building word frequency file.\n");
    FILE *fvocab = fopen(VocabFile,"r");
    FILE *fsave = fopen(WordFreqOutputFile,"w+");
    LINE *Line= (LINE*) malloc(sizeof(LINE));
    double TotalTokenNum = 0.0;
    double freq;
    int end=0;
    // 第一遍遍历所有单词累加counts
    while (1){
        end = GetLines(fvocab,Line);
        if (end == 1) { break;}
        TotalTokenNum += atoll(Line->Count)+0.0;
    }
    // 将文件指针置放回开头
    rewind(fvocab);
    end = 0;
//    fprintf(stderr, "The total num is:%lld",TotalTokenNum);
    // 写入
    while (1) {
        end = GetLines(fvocab,Line);
        if (end == 1) { break;}
        freq = (atoll(Line->Count) / (TotalTokenNum));
        fprintf(fsave,"%s\t%lf.\n",Line->InWord,freq);
    }
    fclose(fvocab);
    fclose(fsave);
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

int main (int args, char**argv) {
    // 变量定义区，第一次阅读可以跳过，
    // 后面用到再反过来看
    int i;
    char VocabFile[100],CoocurFile[100],WordFreqOutputFile[100],ConditionProbFile[100],PMIFile[100];
    // 读取flag
    if ((i = find_flag((char*)"--help",args,argv)) > 0) {
        fprintf(stderr, "Simple tools to calculate the word frequencey, conditional probabilities and PMI between words.\n");
        fprintf(stderr, "mhyao (mhyao@mail.ustc.edu.cn)\n\n");
        fprintf(stderr, "Usage options:\n");
        fprintf(stderr, "\t-VocabFile <char*>\n");
        fprintf(stderr, "\t\tThe path to the pre-prepared vocabulary file, with ('w_i','Counts_i') per line.\n");
        fprintf(stderr, "\t-CoocurFile <char*>\n");
        fprintf(stderr, "\t\tThe path to the pre-prepared cooccurency file, with ('w_i','w_j','Cooccur_ij') per line.\n");
        fprintf(stderr, "\t-WordFreqOutputFile <char*>\n");
        fprintf(stderr, "\t\tThe path to where you save your word freq file with ('w_i', 'p(i)') per line. (freq = Counts_i/TotalTokenNum)\n");
        fprintf(stderr, "\t-ConditionProbFile <char*>\n");
        fprintf(stderr, "\t\tThe path to where you save your conditional probability file with ('w_i', 'w_j', 'p(w_j|w_i)') per line. [p(w_j|w_i) = Cooccur_ij/(sum_k Cooccur_ik)]\n");
        fprintf(stderr, "\t-PMIFile <char*>\n");
        fprintf(stderr, "\t\tThe path to where you save your PMI file with ('w_i', 'w_j', 'pmi(w_i,w_j)') per line. [pmi(w_i,w_j) = p(w_j|w_i) / p (w_j)]\n");
        fprintf(stderr, "Example usage:\n");
        fprintf(stderr, "\t./PMICalculater -VocabFile vocab.txt, -CoocurFile cooccur.txt, -WordFreqOutputFile wordfreq.txt, -ConditionProbFile condiprob.txt, -PMIFile pmi.txt\n");
        return 1;
    }
    if ((i = find_flag((char*)"-VocabFile",args,argv)) > 0) {
        strcpy(VocabFile,argv[i+1]);
        fprintf(stderr, "The VocabFile is:%s",VocabFile);
    }
    if ((i = find_flag((char*)"-CoocurFile",args,argv)) > 0) {
        strcpy(CoocurFile,argv[i+1]);
        fprintf(stderr, "The CoocurFile is:%s",CoocurFile);
    }
    if ((i = find_flag((char*)"-WordFreqOutputFile",args,argv)) > 0) {
        strcpy(WordFreqOutputFile,argv[i+1]);
        fprintf(stderr, "The WordFreqOutputFile is:%s",WordFreqOutputFile);
    }
    if ((i = find_flag((char*)"-ConditionProbFile",args,argv)) > 0) {
        strcpy(ConditionProbFile,argv[i+1]);
        fprintf(stderr, "The ConditionProbFile is:%s",ConditionProbFile);
    }
    if ((i = find_flag((char*)"-PMIFile",args,argv)) > 0) {
        strcpy(PMIFile,argv[i+1]);
        fprintf(stderr, "The PMIFile is:%s",PMIFile);
    }
    // 计算每个单词的词频
    WordFreq(VocabFile,WordFreqOutputFile);
    // 计算每个词对的条件概率
    ConditionalProba(CoocurFile,ConditionProbFile);
    // 计算每个词对的PMI信息
    CalculatePMI(ConditionProbFile,WordFreqOutputFile,PMIFile);
    return 0;
}