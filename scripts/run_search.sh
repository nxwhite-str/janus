#!/bin/bash

script_dir="$(cd -P "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

################################
# Create enrollment templates
################################
root=$script_dir/testdir
metadata=$root/enroll.csv
tuning_data=$root/train.csv
data_path=$script_dir/../data/
sdk_path="$(cd -P "$script_dir/../.." && pwd)"
temp_path=$root/temp_path/
templates_path=$root/search/templates/
templates_list_file=$templates_path/templates.csv
flat_templates_list_file=$templates_path/flat_templates.csv

rm -rf $templates_path/*
echo janus_create_templates $sdk_path $temp_path $data_path $metadata $templates_path $templates_list_file $flat_templates_list_file -algorithm config/D2.2.debug.conf
janus_create_templates $sdk_path $temp_path $data_path $metadata $templates_path $templates_list_file $flat_templates_list_file -algorithm config/D2.2.debug.conf

################################
# Create gallery
################################
gallery_path=$root/search/gallery
flat_gallery_file=$gallery_path/g.flat_gallery

rm -rf $flat_gallery_file
echo janus_create_gallery $sdk_path $temp_path $templates_path $templates_list_file $flat_gallery_file -algorithm config/D2.2.debug.conf -tuning_data $tuning_data -tuning_data_path $data_path
janus_create_gallery $sdk_path $temp_path $templates_path $templates_list_file $flat_gallery_file -algorithm config/D2.2.debug.conf -tuning_data $tuning_data -tuning_data_path $data_path

################################
# Create probe templates
################################
probes=$root/search/probes.csv
probe_templates_path=$root/search/probes/
probe_templates_list_file=$probe_templates_path/templates.csv
probe_flat_templates_list_file=$probe_templates_path/flat_templates.csv

rm -rf $probe_templates_path/*
echo janus_create_templates $sdk_path $temp_path $data_path $probes $probe_templates_path $probe_templates_list_file $probe_flat_templates_list_file -algorithm config/D2.2.debug.conf -verbose -tuning_data $tuning_data
janus_create_templates $sdk_path $temp_path $data_path $probes $probe_templates_path $probe_templates_list_file $probe_flat_templates_list_file -algorithm config/D2.2.debug.conf -verbose -tuning_data $tuning_data

################################
# 1:N search
################################
candlist_file=$root/search/candlists/s.candidate_lists
candlist_length=5

rm -rf $candlist_file
echo janus_search $sdk_path $temp_path $flat_gallery_file $probe_templates_path $probe_flat_templates_list_file $candlist_length $candlist_file -algorithm config/D2.2.debug.conf
janus_search $sdk_path $temp_path $flat_gallery_file $probe_templates_path $probe_flat_templates_list_file $candlist_length $candlist_file -algorithm config/D2.2.debug.conf
