from main import get_all_txt_file
from pathlib import Path


corpus_root_path_dict = {
    'convote_v1.1': [
        '../convote_v1.1/convote_v1.1/data_stage_one',
        '../convote_v1.1/convote_v1.1/data_stage_two',
        '../convote_v1.1/convote_v1.1/data_stage_three',
    ]
}

total_txt_file_path_list = []
for corpus_name in list(corpus_root_path_dict.keys()):
    sub_txt_file_path_list = corpus_root_path_dict[corpus_name]
    for file_path in sub_txt_file_path_list:
        if file_path[-4:] == '.txt':
            total_txt_file_path_list.append(file_path)
        else:
            tmp = get_all_txt_file(Path(file_path))
            total_txt_file_path_list.extend(tmp)
all_file_num = len(total_txt_file_path_list)
all_clean_txt_path = './clean_corpus/bert_torch_corpus.txt'
with open(all_clean_txt_path, 'a+') as f_clean:
    for count, file_path in enumerate(total_txt_file_path_list):
        try:
            with open(file_path, 'r') as f:
                lines = f.readlines()
                for line_id, line in enumerate(lines[:-2]):
                    line_1st = line.strip('\n').lower()
                    line_2st = lines[line_id+1].strip('\n').lower()
                    bert_torch_line = '\t'.join([line_1st, line_2st])
                    f_clean.write(bert_torch_line + '\n')
        except:
            pass
        print(f'文件进度：当前文件id/所有文件数量/文件名称[{count}/{all_file_num} @ {file_path}].')
        f.close()
f_clean.close()
