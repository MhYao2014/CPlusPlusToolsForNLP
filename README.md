# C/C++ Tools For NLP
给自己积累的一些工具，主要是速度快。当然了，我这种菜鸟，限制内存大小的写法还是看不懂的。
## 基本使用（像我这种纯新手，连linux，g++，编译等等东西都稀里糊涂的同学，下面是基本用法）
+ git clone https://github.com/MhYao2014/CPlusPlusToolsForNLP.git
+ cd CPlusPlusToolsForNLP
+ g++ -g BuildVocabAndCooccur.cpp -o BuildVocabAndCooccur
+ ./BuildVocabAndCooccur --help

上述命令行全部在linux的终端跑；windows的赶快在microsoft store上装一个ubuntu子系统，然后在子系统里搞；Mac我没用过。最后像git，g++，exe文件等软件里面一般都会带有--help的flag；先看自带的使用说明，再上网找教程。这里我的exe文件就记录了如何使用的命令行。
## IfBuildVocab == 1时
+ 建立词频表，并保存在文件中。
  + 大写字母一律转化为了小写字母。
  + 标点符号还有其他一些符号（调用了c中的ispunct函数来识别）也都算在了词汇表中，并且还分离了单词和符号连在一起的情况。这一步挺重要的，符合和单词的结合不处理的话，词汇表会变长两倍，算是挺严重的噪声了。大写全变小写也能缩小词汇表。
  + 针对英文。
  + 输入原始corpus就行，不需要过分清洗;
  + 推荐在linux环境下，用g++编译出可执行文件。windows用户推荐使用WSL子系统。
  + 更多feature，请善用--help。
## IfBuildCoocur == 1时
+ 建立共现矩阵（fasttext方式），并保存在文件中。
  + 目前对于过于大的词汇表还无法正常运行。在我的机子上50000词汇表的长度占了28%的内存。我的总内存是多少？我哪里知道，我又不会查。
## ToDo
+ 添加限制内存大小的功能。
+ 添加多线程的版本，将输入语料分割成小块，各自处理。
