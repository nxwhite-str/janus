#!/bin/bash

executable_root=../build/src/utils
################################
# Create enrollment templates
################################
root=testdir
metadata=$root/enroll.csv
tuning_data=$root/train.csv
data_path=../data/
sdk_path=$root/sdk_path/
temp_path=$root/temp_path/
templates_path=$root/search/templates/
templates_list_file=$templates_path/templates.csv
flat_templates_list_file=$templates_path/flat_templates.csv
rm -rf $templates_path/*

./$executable_root/janus_create_templates $sdk_path $temp_path $data_path $metadata $templates_path $templates_list_file $flat_templates_list_file -verbose -tuning_data $tuning_data

################################
# Create gallery
################################
gallery_path=$root/search/gallery
flat_gallery_file=$gallery_path/g.flat_gallery
rm -rf $flat_gallery_file

./$executable_root/janus_create_gallery $sdk_path $temp_path $templates_path $templates_list_file $flat_gallery_file -tuning_data $tuning_data -tuning_data_path $tuning_img_path

################################
# Create probe templates
################################
probes=$root/search/probes.csv
probe_templates_path=$root/search/probes/
probe_templates_list_file=$probe_templates_path/templates.csv
probe_flat_templates_list_file=$probe_templates_path/flat_templates.csv
rm -rf $probe_templates_path/*

# Create probe templates
./$executable_root/janus_create_templates $sdk_path $temp_path $data_path $probes $probe_templates_path $probe_templates_list_file $probe_flat_templates_list_file -verbose -tuning_data $tuning_data

################################
# 1:N search
################################
candlist_file=$root/search/candlists/s.candidate_lists
candlist_length=5
rm -rf $candlist_file

./$executable_root/janus_search $sdk_path $temp_path $flat_gallery_file $probe_templates_path $probe_flat_templates_list_file $candlist_length $candlist_file
