#!/bin/bash

# Check if at least one directory is provided
if [ -z "$1" ]; then
    echo "Usage: $0 [-c] <directory1:virtual_path1> [<directory2:virtual_path2> ...] <HEADER_FILE>"
    echo "  -c: Enable gzip compression for the following directory"
    exit 1
fi

# Last argument is the output header file
HEADER_FILE="${@: -1}"
SRC_FILE="${HEADER_FILE}.c"

# Function to convert file path to a valid C array name
to_c_array_name() {
    local name="${1//[^a-zA-Z0-9_]/_}"
    echo "${name^^}"
}

# Define a map of file extensions to MIME types
declare -A mime_types=(
    ["js"]="application/javascript"
    ["css"]="text/css"
    ["html"]="text/html"
    ["svg"]="image/svg+xml"
    ["txt"]="text/plain"
    ["json"]="application/json"
    ["webmanifest"]="application/json"
    ["xml"]="application/xml"
    ["pdf"]="application/pdf"
    ["jpg"]="image/jpeg"
    ["jpeg"]="image/jpeg"
    ["png"]="image/png"
    ["gif"]="image/gif"
    ["bmp"]="image/bmp"
    ["webp"]="image/webp"
    ["mp3"]="audio/mpeg"
    ["wav"]="audio/wav"
    ["ogg"]="audio/ogg"
    ["mp4"]="video/mp4"
    ["webm"]="video/webm"
    ["avi"]="video/x-msvideo"
    ["zip"]="application/zip"
    ["tar"]="application/x-tar"
    ["gz"]="application/gzip"
    ["bz2"]="application/x-bzip2"
    ["7z"]="application/x-7z-compressed"
    ["rar"]="application/vnd.rar"
    ["doc"]="application/msword"
    ["docx"]="application/vnd.openxmlformats-officedocument.wordprocessingml.document"
    ["xls"]="application/vnd.ms-excel"
    ["xlsx"]="application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
    ["ppt"]="application/vnd.ms-powerpoint"
    ["pptx"]="application/vnd.openxmlformats-officedocument.presentationml.presentation"
    ["odt"]="application/vnd.oasis.opendocument.text"
    ["ods"]="application/vnd.oasis.opendocument.spreadsheet"
    ["odp"]="application/vnd.oasis.opendocument.presentation"
    ["csv"]="text/csv"
    ["tsv"]="text/tab-separated-values"
    ["md"]="text/markdown"
    ["yml"]="application/x-yaml"
    ["yaml"]="application/x-yaml"
    ["ini"]="text/plain"
    ["conf"]="text/plain"
    ["sh"]="application/x-sh"
    ["php"]="application/x-httpd-php"
    ["py"]="text/x-python"
    ["java"]="text/x-java-source"
    ["c"]="text/x-c"
    ["cpp"]="text/x-c++src"
    ["h"]="text/x-c"
    ["hpp"]="text/x-c++hdr"
    ["go"]="text/x-go"
    ["rb"]="text/x-ruby"
    ["pl"]="text/x-perl"
    ["sql"]="application/sql"
    ["log"]="text/plain"
    ["iso"]="application/x-iso9660-image"
    ["dmg"]="application/x-apple-diskimage"
    ["exe"]="application/x-msdownload"
    ["msi"]="application/x-msi"
    ["apk"]="application/vnd.android.package-archive"
    ["deb"]="application/x-deb"
    ["rpm"]="application/x-rpm"
    ["img"]="application/octet-stream"
    ["bin"]="application/octet-stream"
    ["dat"]="application/octet-stream"
    ["ttf"]="font/ttf"
    ["otf"]="font/otf"
    ["woff"]="font/woff"
    ["woff2"]="font/woff2"
    ["eot"]="application/vnd.ms-fontobject"
    ["sfnt"]="font/sfnt"
)

# Default MIME type for unknown extensions
DEFAULT_MIME_TYPE="application/octet-stream"

# Function to get MIME type based on file extension
get_mime_type() {
    local filename="$1"
    local extension="${filename##*.}" # Extract the file extension
    local mime_type="${mime_types[$extension]}"
    echo "${mime_type:-$DEFAULT_MIME_TYPE}" # Use default if extension not found
}

header_guard_name="$(to_c_array_name "$(basename "$HEADER_FILE")")"

# Initialize the header and source files
{
    echo "#ifndef $header_guard_name"
    echo "#define $header_guard_name"
    echo ""
    echo "#include <stdint.h>"
    echo ""
} >"$HEADER_FILE"

{
    echo "#include \"$(basename "$HEADER_FILE")\""
    echo ""
} >"$SRC_FILE"

# Arrays to store file metadata
declare -a files
declare -a files_content
declare -a files_content_lengths
declare -a files_mime_types
declare -a files_compressed

# Function to process a single directory
process_directory() {
    local dir="$1"
    local virtual_prefix="$2"
    local compress="$3" # Whether to compress the files in this directory

    for entry in "$dir"/*; do
        if [ -d "$entry" ]; then
            # If it's a directory, recurse into it
            process_directory "$entry" "$virtual_prefix/$(basename "$entry")" "$compress"
        elif [ -f "$entry" ]; then
            # If it's a file, process it
            local relative_file_path="${virtual_prefix}/$(basename "$entry")"
            local array_name="FS_FILE$(to_c_array_name "$relative_file_path")"

            local content
            local length

            if [ "$compress" = "1" ]; then
                # Compress the file content using gzip and store in a temporary file
                local gzipped_temp_file
                gzipped_temp_file=$(mktemp)
                gzip -c "$entry" >"$gzipped_temp_file"

                # Get the length of the gzipped data in bytes
                length=$(wc -c <"$gzipped_temp_file")

                # Convert the gzipped data to a C array using xxd
                content=$(xxd -i <"$gzipped_temp_file")

                # Clean up the temporary file
                rm "$gzipped_temp_file"
            else
                # Use the file content as-is
                length=$(wc -c <"$entry")
                content=$(xxd -i <"$entry")
            fi

            # Get the MIME type based on the file extension
            local mime_type
            mime_type=$(get_mime_type "$entry")

            # Add comment with the original path
            {
                echo "// Original path: $entry"
                echo "extern const uint8_t $array_name[];"
                echo ""
            } >>"$HEADER_FILE"

            {
                echo "// Original path: $entry"
                echo "const uint8_t $array_name[] = {"
                echo "$content"
                echo "};"
                echo ""
            } >>"$SRC_FILE"

            # Store metadata for arrays
            files+=("\"$relative_file_path\"")
            files_content+=("$array_name")
            files_content_lengths+=("$length")
            files_mime_types+=("\"$mime_type\"")
            files_compressed+=("$compress")
        fi
    done
}

# Function to split argument at the last colon
split_at_last_colon() {
    local arg="$1"
    # Use regex to match the last colon that is not preceded by another colon
    if [[ "$arg" =~ ^(.*[^:]):([^:]+)$ ]]; then
        dir="${BASH_REMATCH[1]}"
        virtual_prefix="${BASH_REMATCH[2]}"
    else
        # If no valid colon is found, treat the entire argument as the directory
        dir="$arg"
        virtual_prefix="/"
    fi
}

# Process each directory argument
compress_next=0
for arg in "${@:1:$#-1}"; do
    if [ "$arg" = "-c" ]; then
        compress_next=1
        continue
    fi

    split_at_last_colon "$arg"
    if [ -z "$dir" ]; then
        echo "Invalid argument: $arg. Expected format: /actual/path:virtual/path"
        exit 1
    fi

    if [[ "$virtual_prefix" == "/" ]]; then
        virtual_prefix=""
    fi

    if [[ "$virtual_prefix" =~ [^/].* ]]; then
        virtual_prefix="/$virtual_prefix"
    fi

    process_directory "$dir" "$virtual_prefix" "$compress_next"
    compress_next=0
done

# Write the `fs_files` array
{
    echo "extern const char *fs_files[];"
    echo ""
} >>"$HEADER_FILE"

{
    echo "const char *fs_files[] = {"
    for file in "${files[@]}"; do
        echo "    $file,"
    done
    echo "};"
    echo ""
} >>"$SRC_FILE"

# Write the `fs_files_count` constant
{
    echo "extern const uint32_t fs_files_count;"
    echo ""
} >>"$HEADER_FILE"

{
    echo "const uint32_t fs_files_count = ${#files[@]};"
    echo ""
} >>"$SRC_FILE"

# Write the `fs_files_content` array
{
    echo "extern const uint8_t *fs_files_content[];"
    echo ""
} >>"$HEADER_FILE"

{
    echo "const uint8_t *fs_files_content[] = {"
    for content in "${files_content[@]}"; do
        echo "    $content,"
    done
    echo "};"
    echo ""
} >>"$SRC_FILE"

# Write the `fs_files_content_lengths` array
total_size=0

{
    echo "extern const uint32_t fs_files_content_lengths[];"
    echo ""
} >>"$HEADER_FILE"

{
    echo "const uint32_t fs_files_content_lengths[] = {"
    for length in "${files_content_lengths[@]}"; do
        total_size=$((total_size + length))
        echo "    $length,"
    done
    echo "};"
    echo "// total size $(numfmt --to=iec $total_size)"
    echo ""
} >>"$SRC_FILE"

{
    echo "// total size $(numfmt --to=iec $total_size)"
    echo ""
} >>"$HEADER_FILE"

# Write the `fs_files_mime_types` array
{
    echo "extern const char *fs_files_mime_types[];"
    echo ""
} >>"$HEADER_FILE"

{
    echo "const char *fs_files_mime_types[] = {"
    for mime_type in "${files_mime_types[@]}"; do
        echo "    $mime_type,"
    done
    echo "};"
    echo ""
} >>"$SRC_FILE"

# Write the `fs_files_compressed` array
{
    echo "extern const uint8_t fs_files_compressed[];"
    echo ""
} >>"$HEADER_FILE"

{
    echo "const uint8_t fs_files_compressed[] = {"
    for compressed in "${files_compressed[@]}"; do
        echo "    $compressed,"
    done
    echo "};"
    echo ""
} >>"$SRC_FILE"

# Close the header file
echo "#endif // $header_guard_name" >>"$HEADER_FILE"

echo "Header file generated at: $HEADER_FILE"
echo "Source file generated at: $SRC_FILE"