#!/bin/bash

executable_root=../build/src/utils
################################
# Create templates
################################
root=testdir
metadata=$root/enroll.csv
tuning_data=$root/train.csv
data_path=../data/
sdk_path=$root/sdk_path/
temp_path=$root/temp_path/
templates_path=$root/verification/templates/
templates_list_file=$templates_path/templates.csv
flat_templates_list_file=$templates_path/flat_templates.csv
rm -rf $templates_path/*

./$executable_root/janus_create_templates $sdk_path $temp_path $data_path $metadata $templates_path $templates_list_file $flat_templates_list_file -verbose -tuning_data $tuning_data

################################
# 1:1 verification
################################
verif_path=$root/verification
templates_pairs_file=$verif_path/verif_pairs.csv
scores_file=$verif_path/scores/verify.scores
rm -rf $scores_file

./$executable_root/janus_verify $sdk_path $temp_path $templates_path $templates_pairs_file $scores_file
