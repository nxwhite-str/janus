// These file is designed to have no dependencies outside the C++ Standard Library
#include <algorithm>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <assert.h>
#include <iterator>

#include <iarpa_janus_io.h>

using namespace std;

#define ENUM_CASE(X) case JANUS_##X: return #X;
#define ENUM_COMPARE(X,Y) if (!strcmp(#X, Y)) return JANUS_##X;

const char *janus_error_to_string(janus_error error)
{
    switch (error) {
        ENUM_CASE(SUCCESS)
        ENUM_CASE(UNKNOWN_ERROR)
        ENUM_CASE(OUT_OF_MEMORY)
        ENUM_CASE(INVALID_SDK_PATH)
        ENUM_CASE(OPEN_ERROR)
        ENUM_CASE(READ_ERROR)
        ENUM_CASE(WRITE_ERROR)
        ENUM_CASE(PARSE_ERROR)
        ENUM_CASE(INVALID_MEDIA)
        ENUM_CASE(MISSING_TEMPLATE_ID)
        ENUM_CASE(MISSING_FILE_NAME)
        ENUM_CASE(NULL_ATTRIBUTES)
        ENUM_CASE(MISSING_ATTRIBUTES)
        ENUM_CASE(FAILURE_TO_DETECT)
        ENUM_CASE(FAILURE_TO_ENROLL)
        ENUM_CASE(FAILURE_TO_SERIALIZE)
        ENUM_CASE(FAILURE_TO_DESERIALIZE)
        ENUM_CASE(NUM_ERRORS)
        ENUM_CASE(NOT_IMPLEMENTED)
    }
    return "UNKNOWN_ERROR";
}

janus_error janus_error_from_string(const char *error)
{
    ENUM_COMPARE(SUCCESS, error)
    ENUM_COMPARE(UNKNOWN_ERROR, error)
    ENUM_COMPARE(OUT_OF_MEMORY, error)
    ENUM_COMPARE(INVALID_SDK_PATH, error)
    ENUM_COMPARE(OPEN_ERROR, error)
    ENUM_COMPARE(READ_ERROR, error)
    ENUM_COMPARE(WRITE_ERROR, error)
    ENUM_COMPARE(PARSE_ERROR, error)
    ENUM_COMPARE(INVALID_MEDIA, error)
    ENUM_COMPARE(MISSING_TEMPLATE_ID, error)
    ENUM_COMPARE(MISSING_FILE_NAME, error)
    ENUM_COMPARE(NULL_ATTRIBUTES, error)
    ENUM_COMPARE(MISSING_ATTRIBUTES, error)
    ENUM_COMPARE(FAILURE_TO_DETECT, error)
    ENUM_COMPARE(FAILURE_TO_ENROLL, error)
    ENUM_COMPARE(FAILURE_TO_SERIALIZE, error)
    ENUM_COMPARE(FAILURE_TO_DESERIALIZE, error)
    ENUM_COMPARE(NUM_ERRORS, error)
    ENUM_COMPARE(NOT_IMPLEMENTED, error)
    return JANUS_UNKNOWN_ERROR;
}

// For computing metrics
static vector<double> janus_load_media_samples;
static vector<double> janus_free_media_samples;
static vector<double> janus_detection_samples;
static vector<double> janus_create_template_samples;
static vector<double> janus_template_size_samples;
static vector<double> janus_serialize_template_samples;
static vector<double> janus_deserialize_template_samples;
static vector<double> janus_delete_serialized_template_samples;
static vector<double> janus_delete_template_samples;
static vector<double> janus_verify_samples;
static vector<double> janus_create_gallery_samples;
static vector<double> janus_prepare_gallery_samples;
static vector<double> janus_gallery_size_samples;
static vector<double> janus_gallery_insert_samples;
static vector<double> janus_gallery_remove_samples;
static vector<double> janus_serialize_gallery_samples;
static vector<double> janus_deserialize_gallery_samples;
static vector<double> janus_delete_serialized_gallery_samples;
static vector<double> janus_delete_gallery_samples;
static vector<double> janus_search_samples;
static int janus_missing_attributes_count = 0;
static int janus_failure_to_detect_count = 0;
static int janus_failure_to_enroll_count = 0;
static int janus_other_errors_count = 0;

static void _janus_add_sample(vector<double> &samples, double sample);

#ifndef JANUS_CUSTOM_ADD_SAMPLE

static void _janus_add_sample(vector<double> &samples, double sample)
{
    samples.push_back(sample);
}

#endif // JANUS_CUSTOM_ADD_SAMPLE

#ifndef JANUS_CUSTOM_DETECT

janus_error janus_detect_helper(const string &data_path, janus_metadata metadata, const size_t min_face_size, const string &detection_list_file, bool verbose)
{
    clock_t start;

    ifstream file(metadata);
    ofstream output(detection_list_file);

    output << "FILENAME,FACE_X,FACE_Y,FACE_WIDTH,FACE_HEIGHT,DETECTION_CONFIDENCE" << endl;

    // Parse the header
    string line;
    // NLC: No header on CS3 face detection csv file
    // getline(file, line);
    size_t num_faces = 0;
    while (getline(file, line)) {
      ++num_faces;
    }

    file.clear();
    file.seekg(0);

    size_t face_cnt = 0;
    while (getline(file, line)) {
        cout << "[janus_detect_helper]: " << ++face_cnt  << "/" << num_faces << endl;
        istringstream attributes(line);
        string filename;
        getline(attributes, filename, ',');

        janus_media media;

        start = clock();
        JANUS_ASSERT(janus_load_media(data_path + filename, media))
        _janus_add_sample(janus_load_media_samples, 1000.0 * (clock() - start) / CLOCKS_PER_SEC);

        vector<janus_track> tracks;

        start = clock();
        janus_error error = janus_detect(media, min_face_size, tracks);
        _janus_add_sample(janus_detection_samples, 1000.0 * (clock() - start) / CLOCKS_PER_SEC);

        if (error == JANUS_FAILURE_TO_DETECT) {
            janus_failure_to_detect_count++;
            continue;
        } else if (error != JANUS_SUCCESS) {
            janus_other_errors_count++;
            continue;
        }

        for (size_t i = 0; i < tracks.size(); i++) {
            const janus_track &track = tracks[i];
            for (size_t j = 0; j < track.track.size(); j++) {
                const janus_attributes &attrs = track.track[j];
                output << filename << ","
                       << attrs.face_x << ","
                       << attrs.face_y << ","
                       << attrs.face_width << ","
                       << attrs.face_height << ","
                       << track.detection_confidence << "\n";
            }
        }
    }

    file.close();
    output.close();

    if (verbose)
        janus_print_metrics(janus_get_metrics());

    return JANUS_SUCCESS;
}

#endif // JANUS_CUSTOM_DETECT

struct TemplateData
{
    vector<string> filenames;
    vector<janus_template_id> templateIDs;
    map<janus_template_id, int> subjectIDLUT;
    vector<janus_track> tracks;

    void release()
    {
        filenames.clear();
        templateIDs.clear();
        subjectIDLUT.clear();
        tracks.clear();
    }
};

struct TemplateIterator : public TemplateData
{
    size_t i;
    bool verbose;

    TemplateIterator(janus_metadata metadata, bool verbose)
        : i(0), verbose(verbose)
    {
        ifstream file(metadata);

        // Parse header
        string line;
        getline(file, line);
        istringstream attributeNames(line);
        string attributeName;
        getline(attributeNames, attributeName, ','); // TEMPLATE_ID
        getline(attributeNames, attributeName, ','); // SUBJECT_ID
        getline(attributeNames, attributeName, ','); // FILE_NAME
        vector<string> header;
        while (getline(attributeNames, attributeName, ','))
            header.push_back(attributeName);

        // Parse rows
        while (getline(file, line)) {
            istringstream attributeValues(line);
            string templateID, subjectID, filename, attributeValue;
            getline(attributeValues, templateID, ',');
            getline(attributeValues, subjectID, ',');
            getline(attributeValues, filename, ',');
            templateIDs.push_back(atoi(templateID.c_str()));
            subjectIDLUT.insert(make_pair(atoi(templateID.c_str()), atoi(subjectID.c_str())));
            filenames.push_back(filename);

            // Construct a track from the metadata
            janus_track track;
            janus_attributes attributes;
            for (int j = 0; getline(attributeValues, attributeValue, ','); j++) {
                double value = attributeValue.empty() ? NAN : atof(attributeValue.c_str());
                if (header[j] == "FRAME_NUMBER")
                    attributes.frame_number = value;
                else if (header[j] == "FACE_X")
                    attributes.face_x = value;
                else if (header[j] == "FACE_Y")
                    attributes.face_y = value;
                else if (header[j] == "FACE_WIDTH")
                    attributes.face_width = value;
                else if (header[j] == "FACE_HEIGHT")
                    attributes.face_height = value;
                else if (header[j] == "RIGHT_EYE_X")
                    attributes.right_eye_x = value;
                else if (header[j] == "RIGHT_EYE_Y")
                    attributes.right_eye_y = value;
                else if (header[j] == "LEFT_EYE_X")
                    attributes.left_eye_x = value;
                else if (header[j] == "LEFT_EYE_Y")
                    attributes.left_eye_y = value;
                else if (header[j] == "NOSE_BASE_X")
                    attributes.nose_base_x = value;
                else if (header[j] == "NOSE_BASE_Y")
                    attributes.nose_base_y = value;
                else if (header[j] == "FACE_YAW")
                    attributes.face_yaw = value;
                else if (header[j] == "FOREHEAD_VISIBLE")
                    attributes.forehead_visible = value;
                else if (header[j] == "EYES_VISIBLE")
                    attributes.eyes_visible = value;
                else if (header[j] == "NOSE_MOUTH_VISIBLE")
                    attributes.nose_mouth_visible = value;
                else if (header[j] == "INDOOR")
                    attributes.indoor = value;
                else if (header[j] == "GENDER")
                    track.gender = value;
                else if (header[j] == "AGE")
                    track.age = value;
                else if (header[j] == "SKIN_TONE")
                    track.skin_tone = value;
            }
            track.track.push_back(attributes);
            tracks.push_back(track);
        }
        if (verbose)
            fprintf(stderr, "\rEnrolling %zu/%zu", i, tracks.size());
    }

    TemplateData next()
    {
        TemplateData templateData;
        if (i >= tracks.size()) {
            fprintf(stderr, "\n");
        } else {
            const janus_template_id templateID = templateIDs[i];
            while ((i < tracks.size()) && (templateIDs[i] == templateID)) {
                templateData.templateIDs.push_back(templateIDs[i]);
                templateData.filenames.push_back(filenames[i]);
                templateData.tracks.push_back(tracks[i]);
                i++;
            }
            if (verbose)
                fprintf(stderr, "\rEnrolling %zu/%zu", i, tracks.size());
        }
        return templateData;
    }

    static janus_error create(const string &data_path, const TemplateData templateData, const janus_template_role role, janus_template *template_, janus_template_id *templateID, bool verbose)
    {
        clock_t start;

        // A set to hold all of the media and metadata required to make a full template
        vector<janus_association> associations;

        // Create a set of all the media used for this template
        for (size_t i = 0; i < templateData.templateIDs.size(); i++) {
            janus_media media;

            start = clock();
            JANUS_ASSERT(janus_load_media(data_path + templateData.filenames[i], media))
            _janus_add_sample(janus_load_media_samples, 1000.0 * (clock() - start) / CLOCKS_PER_SEC);

            janus_association association;
            association.media = media;
            association.metadata = templateData.tracks[i];
            associations.push_back(association);
        }

        // Create the template
        start = clock();
        janus_error error = janus_create_template(associations, role, *template_);
        _janus_add_sample(janus_create_template_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);

        // Check the result for errors
        if (error == JANUS_MISSING_ATTRIBUTES) {
            janus_missing_attributes_count++;
            if (verbose)
                printf("Missing attributes for: %s\n", templateData.filenames[0].c_str());
        } else if (error == JANUS_FAILURE_TO_ENROLL) {
            janus_failure_to_enroll_count++;
            if (verbose)
                printf("Failure to enroll: %s\n", templateData.filenames[0].c_str());
        } else if (error != JANUS_SUCCESS) {
            janus_other_errors_count++;
            printf("Warning: %s on: %s\n", janus_error_to_string(error),templateData.filenames[0].c_str());
        }

        // Free the media
        for (size_t i = 0; i < associations.size(); i++) {
            start = clock();
            JANUS_ASSERT(janus_free_media(associations[i].media));
            _janus_add_sample(janus_free_media_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);
        }

        *templateID = templateData.templateIDs[0];
        return JANUS_SUCCESS;
    }

    ~TemplateIterator() { release(); }
};

#ifndef JANUS_CUSTOM_CREATE_TEMPLATES

janus_error janus_create_templates_helper(const string &data_path, janus_metadata metadata, const string &templates_path, const string &templates_list_file, const janus_template_role role, bool verbose)
{
    clock_t start;

    // Create an iterator to loop through the templates
    TemplateIterator ti(metadata, true);

    // Preallocate some variables
    janus_template template_;
    janus_template_id templateID;

    // Set up file I/O
    ofstream templates_list_stream(templates_list_file.c_str(), ios::out | ios::ate);

    TemplateData templateData = ti.next();
    while (!templateData.templateIDs.empty()) {
        JANUS_CHECK(TemplateIterator::create(data_path, templateData, role, &template_, &templateID, verbose))

        // Useful strings
        char templateIDBuffer[10], subjectIDBuffer[10];
        sprintf(templateIDBuffer, "%zu", templateID);
        const string templateIDString(templateIDBuffer);
        sprintf(subjectIDBuffer, "%d", templateData.subjectIDLUT[templateID]);
        const string subjectIDString(subjectIDBuffer);
        const string templateOutputFile = templates_path + templateIDString + ".template";

        // Serialize the template to a file.
        ofstream template_stream(templateOutputFile.c_str(), ios::out | ios::binary);
        start = clock();
        JANUS_CHECK(janus_serialize_template(template_, template_stream));
        _janus_add_sample(janus_serialize_template_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);
        template_stream.close();

        // Write the template metadata to the list
        templates_list_stream << templateIDString << "," << subjectIDString << "," << templateOutputFile << "\n";

        // Delete the actual template
        start = clock();
        JANUS_CHECK(janus_delete_template(template_));
        _janus_add_sample(janus_delete_template_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);

        // Move to the next template
        templateData = ti.next();
    }
    templates_list_stream.close();

    if (verbose)
        janus_print_metrics(janus_get_metrics());

    return JANUS_SUCCESS;
}

#endif // JANUS_CUSTOM_CREATE_TEMPLATES

static janus_error janus_load_templates_from_file(const string &templates_list_file, vector<janus_template> &templates, vector<janus_template_id> &template_ids, vector<int> &subject_ids)
{
    clock_t start;

    ifstream templates_list_stream(templates_list_file.c_str());
    string line;

    while (getline(templates_list_stream, line)) {
        istringstream row(line);
        string template_id, subject_id, template_file;
        getline(row, template_id, ',');
        getline(row, subject_id, ',');
        getline(row, template_file, ',');

        template_ids.push_back(atoi(template_id.c_str()));
        subject_ids.push_back(atoi(subject_id.c_str()));

        // Load the serialized template from disk
        ifstream template_stream(template_file.c_str(), ios::in | ios::binary);
        janus_template template_ = NULL;
        start = clock();
        JANUS_CHECK(janus_deserialize_template(template_, template_stream));
        _janus_add_sample(janus_deserialize_template_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);
        template_stream.close();

        templates.push_back(template_);
    }
    templates_list_stream.close();

    return JANUS_SUCCESS;
}

#ifndef JANUS_CUSTOM_CREATE_GALLERY

janus_error janus_create_gallery_helper(const string &templates_list_file, const string &gallery_file, bool verbose)
{
    clock_t start;

    vector<janus_template> templates;
    vector<janus_template_id> template_ids;
    vector<int> subject_ids;
    JANUS_CHECK(janus_load_templates_from_file(templates_list_file, templates, template_ids, subject_ids));

    // Create the gallery
    janus_gallery gallery = NULL;
    start = clock();
    janus_create_gallery(templates, template_ids, gallery);
    _janus_add_sample(janus_create_gallery_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);

    // Prepare the gallery for searching
    start = clock();
    janus_prepare_gallery(gallery);
    _janus_add_sample(janus_prepare_gallery_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);

    // Serialize the gallery to a file.
    ofstream gallery_stream(gallery_file.c_str(), ios::out | ios::binary);
    start = clock();
    JANUS_CHECK(janus_serialize_gallery(gallery, gallery_stream));
    _janus_add_sample(janus_serialize_gallery_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);
    gallery_stream.close();

    // Delete the actual gallery
    start = clock();
    JANUS_CHECK(janus_delete_gallery(gallery));
    _janus_add_sample(janus_delete_gallery_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);

    if (verbose)
        janus_print_metrics(janus_get_metrics());

    return JANUS_SUCCESS;
}

#endif // JANUS_CUSTOM_CREATE_GALLERY

#ifndef JANUS_CUSTOM_VERIFY

janus_error janus_verify_helper(const string &templates_list_file_a, const string &templates_list_file_b, const string &scores_file, bool verbose)
{
    clock_t start;

    // Load the template sets
    vector<janus_template> templates_a, templates_b;
    vector<janus_template_id> template_ids_a, template_ids_b;
    vector<int> subject_ids_a, subject_ids_b;

    JANUS_CHECK(janus_load_templates_from_file(templates_list_file_a, templates_a, template_ids_a, subject_ids_a));
    JANUS_CHECK(janus_load_templates_from_file(templates_list_file_b, templates_b, template_ids_b, subject_ids_b));

    assert(templates_a.size() == templates_b.size());

    // Compare the templates and write the results to the scores file
    ofstream scores_stream(scores_file.c_str(), ios::out | ios::ate);
    for (size_t i = 0; i < templates_a.size(); i++) {
        double similarity;
        start = clock();
        janus_verify(templates_a[i], templates_b[i], similarity);
        _janus_add_sample(janus_verify_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);

        scores_stream << template_ids_a[i] << "," << template_ids_b[i] << "," << similarity << ","
                      << (subject_ids_a[i] == subject_ids_b[i] ? "true" : "false") << "\n";
    }
    scores_stream.close();

    if (verbose)
        janus_print_metrics(janus_get_metrics());

    return JANUS_SUCCESS;
}

#endif // JANUS_CUSTOM_VERIFY

#ifndef JANUS_CUSTOM_SEARCH

janus_error janus_ensure_size(const vector<janus_template_id> &all_ids, vector<janus_template_id> &return_ids, vector<double> &similarities)
{
    set<janus_template_id> return_lookup(return_ids.begin(), return_ids.end());

    return_ids.reserve(all_ids.size()); similarities.reserve(all_ids.size());
    for (size_t i = 0; i < all_ids.size(); i++) {
        janus_template_id id = all_ids[i];
        if (return_lookup.find(id) == return_lookup.end()) {
            return_ids.push_back(id);
            similarities.push_back(0.0);
        }
    }

    return JANUS_SUCCESS;
}

janus_error janus_search_helper(const string &probes_list_file, const string &gallery_list_file, const string &gallery_file, int num_requested_returns, const string &candidate_list_file, bool verbose)
{
    clock_t start;

    // Vectors to hold loaded data
    vector<janus_template> probe_templates, gallery_templates;
    vector<janus_template_id> probe_template_ids, gallery_template_ids;
    vector<int> probe_subject_ids, gallery_subject_ids;

    JANUS_CHECK(janus_load_templates_from_file(probes_list_file, probe_templates, probe_template_ids, probe_subject_ids));
    JANUS_CHECK(janus_load_templates_from_file(gallery_list_file, gallery_templates, gallery_template_ids, gallery_subject_ids))

    // Build template_id -> subject_id LUT for the gallery
    map<janus_template_id, int> subjectIDLUT;
    for (size_t i = 0; i < gallery_template_ids.size(); i++) {
        subjectIDLUT.insert(make_pair(gallery_template_ids[i], gallery_subject_ids[i]));

        start = clock();
        JANUS_CHECK(janus_delete_template(gallery_templates[i]))
        _janus_add_sample(janus_delete_template_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);
    }

    // Load the serialized gallery from disk
    ifstream gallery_stream(gallery_file.c_str(), ios::in | ios::binary);
    janus_gallery gallery = NULL;
    start = clock();
    JANUS_CHECK(janus_deserialize_gallery(gallery, gallery_stream));
    _janus_add_sample(janus_deserialize_gallery_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);

    ofstream candidate_stream(candidate_list_file.c_str(), ios::out | ios::ate);
    std::vector<std::string> headers = {
      "SEARCH_TEMPLATE_ID",
      "CANDIDATE_RANK",
      "ENROLL_TEMPLATE_ID",
      "SIMILARITY_SCORE",
      "PROBE_SUBJECT_ID",
      "GALLERY_SUBJECT_ID",
      "Y",
    };

    copy(begin(headers), end(headers), ostream_iterator<string>(candidate_stream, " "));
    candidate_stream << endl;
    for (size_t i = 0; i < probe_templates.size(); i++) {
        vector<janus_template_id> return_template_ids;
        vector<double> similarities;
        start = clock();
        JANUS_CHECK(janus_search(probe_templates[i], gallery, num_requested_returns, return_template_ids, similarities));
        _janus_add_sample(janus_search_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);

        janus_ensure_size(gallery_template_ids, return_template_ids, similarities);

        // for (size_t j = 0; j < return_template_ids.size(); j++)
        //     candidate_stream << probe_template_ids[i] << "," << j << "," << return_template_ids[j] << "," << similarities[j]
        //                      << "," << (probe_subject_ids[i] == subjectIDLUT[return_template_ids[j]] ? "true" : "false") << "\n";

        for (size_t j = 0; j < return_template_ids.size(); j++)
            candidate_stream << probe_template_ids[i] << " " << j << " " << return_template_ids[j] << " " << similarities[j]
                             << " " << probe_subject_ids[i]
                             << " " << subjectIDLUT[return_template_ids[j]]
                             << " " << (probe_subject_ids[i] == subjectIDLUT[return_template_ids[j]] ? "1.0" : "0.0") << "\n";

        start = clock();
        JANUS_CHECK(janus_delete_template(probe_templates[i]))
        _janus_add_sample(janus_delete_template_samples, 1000 * (clock() - start) / CLOCKS_PER_SEC);
    }
    candidate_stream.close();

    if (verbose)
        janus_print_metrics(janus_get_metrics());

    return JANUS_SUCCESS;
}

#endif // JANUS_CUSTOM_SEARCH

static janus_metric calculateMetric(const vector<double> &samples)
{
    janus_metric metric;
    metric.count = samples.size();

    if (metric.count > 0) {
        metric.mean = 0;
        for (size_t i = 0; i < samples.size(); i++)
            metric.mean += samples[i];
        metric.mean /= samples.size();

        metric.stddev = 0;
        for (size_t i = 0; i < samples.size(); i++)
            metric.stddev += pow(samples[i] - metric.mean, 2.0);
        metric.stddev = sqrt(metric.stddev / samples.size());
    } else {
        metric.mean = numeric_limits<double>::quiet_NaN();
        metric.stddev = numeric_limits<double>::quiet_NaN();
    }

    return metric;
}

janus_metrics janus_get_metrics()
{
    janus_metrics metrics;
    metrics.janus_load_media_speed                 = calculateMetric(janus_load_media_samples);
    metrics.janus_free_media_speed                 = calculateMetric(janus_free_media_samples);
    metrics.janus_detection_speed                  = calculateMetric(janus_detection_samples);
    metrics.janus_create_template_speed            = calculateMetric(janus_create_template_samples);
    metrics.janus_template_size                    = calculateMetric(janus_template_size_samples);
    metrics.janus_serialize_template_speed         = calculateMetric(janus_serialize_template_samples);
    metrics.janus_deserialize_template_speed       = calculateMetric(janus_deserialize_template_samples);
    metrics.janus_delete_serialized_template_speed = calculateMetric(janus_delete_serialized_template_samples);
    metrics.janus_delete_template_speed            = calculateMetric(janus_delete_template_samples);
    metrics.janus_verify_speed                     = calculateMetric(janus_verify_samples);
    metrics.janus_create_gallery_speed             = calculateMetric(janus_create_gallery_samples);
    metrics.janus_prepare_gallery_speed            = calculateMetric(janus_prepare_gallery_samples);
    metrics.janus_gallery_size                     = calculateMetric(janus_gallery_size_samples);
    metrics.janus_gallery_insert_speed             = calculateMetric(janus_gallery_insert_samples);
    metrics.janus_gallery_remove_speed             = calculateMetric(janus_gallery_remove_samples);
    metrics.janus_serialize_gallery_speed          = calculateMetric(janus_serialize_gallery_samples);
    metrics.janus_deserialize_gallery_speed        = calculateMetric(janus_deserialize_gallery_samples);
    metrics.janus_delete_serialized_gallery_speed  = calculateMetric(janus_delete_serialized_gallery_samples);
    metrics.janus_delete_gallery_speed             = calculateMetric(janus_delete_gallery_samples);
    metrics.janus_search_speed                     = calculateMetric(janus_search_samples);
    metrics.janus_missing_attributes_count         = janus_missing_attributes_count;
    metrics.janus_failure_to_enroll_count          = janus_failure_to_enroll_count;
    metrics.janus_other_errors_count               = janus_other_errors_count;
    return metrics;
}

static void printMetric(FILE *file, const char *name, janus_metric metric, bool speed = true)
{
    if (metric.count > 0)
        fprintf(file, "%s\t%.2g\t%.2g\t%s\t%.2g\n", name, metric.mean, metric.stddev, speed ? "ms" : "KB", double(metric.count));
}

void janus_print_metrics(janus_metrics metrics)
{
    fprintf(stderr,     "API Symbol                      \tMean\tStdDev\tUnits\tCount\n");
    printMetric(stderr, "janus_load_media                ", metrics.janus_load_media_speed);
    printMetric(stderr, "janus_free_media                ", metrics.janus_free_media_speed);
    printMetric(stderr, "janus_detection                 ", metrics.janus_detection_speed);
    printMetric(stderr, "janus_create_template           ", metrics.janus_create_template_speed);
    printMetric(stderr, "janus_template_size             ", metrics.janus_template_size, false);
    printMetric(stderr, "janus_serialize_template        ", metrics.janus_serialize_template_speed);
    printMetric(stderr, "janus_deserialize_template      ", metrics.janus_deserialize_template_speed);
    printMetric(stderr, "janus_delete_serialized_template", metrics.janus_delete_serialized_template_speed);
    printMetric(stderr, "janus_delete_template           ", metrics.janus_delete_template_speed);
    printMetric(stderr, "janus_verify                    ", metrics.janus_verify_speed);
    printMetric(stderr, "janus_create_gallery            ", metrics.janus_create_gallery_speed);
    printMetric(stderr, "janus_prepare_gallery           ", metrics.janus_prepare_gallery_speed);
    printMetric(stderr, "janus_gallery_size              ", metrics.janus_gallery_size, false);
    printMetric(stderr, "janus_gallery_insert            ", metrics.janus_gallery_insert_speed);
    printMetric(stderr, "janus_gallery_remove            ", metrics.janus_gallery_remove_speed);
    printMetric(stderr, "janus_serialize_gallery         ", metrics.janus_serialize_gallery_speed);
    printMetric(stderr, "janus_deserialize_gallery       ", metrics.janus_deserialize_gallery_speed);
    printMetric(stderr, "janus_delete_serialized_gallery ", metrics.janus_delete_serialized_gallery_speed);
    printMetric(stderr, "janus_delete_gallery            ", metrics.janus_delete_gallery_speed);
    printMetric(stderr, "janus_search                    ", metrics.janus_search_speed);
    fprintf(stderr,     "\n\n");
    fprintf(stderr,     "janus_error                     \tCount\n");
    fprintf(stderr,     "JANUS_MISSING_ATTRIBUTES        \t%d\n", metrics.janus_missing_attributes_count);
    fprintf(stderr,     "JANUS_FAILURE_TO_ENROLL         \t%d\n", metrics.janus_failure_to_enroll_count);
    fprintf(stderr,     "All other errors                \t%d\n", metrics.janus_other_errors_count);
}
