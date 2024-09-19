#!/bin/bash

if [ "$#" -ne 2 ] || [ "$1" != "-i" ]; then
  echo "Usage: $0 -i <input_file>"
  exit 1
fi

INPUT_FILE="$2"
if [ ! -f "$INPUT_FILE" ]; then
  echo "Input file not found!"
  exit 1
fi

mapfile -t input_line < "$INPUT_FILE"

all_formats=("zip" "rar" "tar")
all_language=("c" "cpp" "python" "sh")

check_integer() {
    local num=$1
    if ((num)) 2>/dev/null || [ "$num" -eq 0 ] 2>/dev/null; then
        return 0
    else
        return 1
    fi
}


terminate_with_error() {
  echo "Invalid input file format"
  exit 1
}

if [ "${#input_line[@]}" -ne 11 ]; then
    terminate_with_error
fi


is_archived="${input_line[0]}"
allowed_formats="${input_line[1]}"
allowed_languages="${input_line[2]}"
total_marks="${input_line[3]}"
penalty_unmatched_output="${input_line[4]}"
working_directory="${input_line[5]}"
student_id_range=(${input_line[6]})
expected_output_file="${input_line[7]}"
penalty_guidelines="${input_line[8]}"
plagiarism_file="${input_line[9]}"
plagiarism_penalty="${input_line[10]}"


validate_input() {
    if [ $is_archived != "true" ] && [ $is_archived != "false" ]; then
        # echo "49"
        terminate_with_error
    elif [ ! -d "$working_directory" ] || [ ! -f "$expected_output_file" ] || [ ! -f "$plagiarism_file" ]; then
        # echo "52"
        terminate_with_error
    fi
    #check if allowed_formats are subset of all_formats
    for format in $allowed_formats; do
        local found=0
        # echo "$format"
        for f in "${all_formats[@]}"; do
            if [ "$format" = "$f" ]; then
                found=1
                break
            fi
        done
        if [ $found -eq 0 ]; then
            terminate_with_error
        fi
    done

    for lang in $allowed_languages; do
        local found=0
        for l in "${all_language[@]}"; do
            if [ "$lang" = "$l" ]; then
                found=1
                break
            fi
        done
        if [ $found -eq 0 ]; then
            terminate_with_error
        fi
    done
    check_integer $total_marks
    local a=$?
    check_integer $penalty_unmatched_output
    local b=$?
    check_integer $penalty_guidelines
    local c=$?
    check_integer $plagiarism_penalty
    local d=$?
    ret=$((a+b+c+d))
    if [ $ret -ne 0 ]; then
        terminate_with_error
    fi
}

create_directories() {
  mkdir -p issues checked temporary
  rm -rf issues/* checked/* temporary/*
}

validate_input
create_directories

mapfile -t plagiarism_content < "$plagiarism_file"
mapfile -t expected_output_content < "$expected_output_file"

start_id=${student_id_range[0]}
end_id=${student_id_range[1]}

echo "id,marks,marks_deducted,total_marks,remarks" > marks.csv

# $1 = student_id
has_copied()
{
    local student_id=$1
    for line in "${plagiarism_content[@]}"; do
        if [ "$line" = "$student_id" ]; then
            echo "true"
            return 0
        fi
    done
    echo "false"
}


# $1 = submission_file, $2 = id
unarchive() 
{
    local submission_file=$1
    local id=$2
    local filename=$(basename "$submission_file")
    local extension="${submission_file##*.}"
    # echo "extension: $extension"
    if [[ ! " $allowed_formats " =~ " $extension " ]]; then
        echo "$id,0,10,-10,issue case #2" >> marks.csv
        mv "$submission_file" issues/
        return 1
    fi

    cp "$submission_file" temporary/

    if [ "$extension" = "zip" ]; then
        unzip "$submission_file" -d temporary/ > /dev/null 2>&1
    elif [ "$extension" = "rar" ]; then
        unrar x "$submission_file" temporary/ > /dev/null 2>&1
    elif [ "$extension" = "tar" ]; then
        tar -xvf "$submission_file" -C temporary/ > /dev/null 2>&1
    fi
    rm "temporary/$filename"
    return 0
}

# $1 = submission_file, $2 = id
generate_output()
{
    local submission_file=$1
    local id=$2
    local filename=$(basename "$submission_file")
    local extension="${submission_file##*.}"
    local parentdir=$(dirname "$submission_file")
    local ext=$extension
    if [ $extension = "py" ];then
        ext="python"
    fi
    if [[ ! " $allowed_languages " =~ " $ext " ]]; then
        return 2
    fi
    # echo "parentdir: $parentdir"
    if [ "$extension" = "c" ]; then
        gcc "$submission_file" -o "$submission_file.out" > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            return 1
        fi
        "./$submission_file.out" > "$parentdir/${id}_output.txt"
        rm "$submission_file.out"
        # mv "$parentdir" "checked/"
    elif [ "$extension" = "cpp" ]; then
        g++ "$submission_file" -o "$submission_file.out" > /dev/null 2>&1
        if [ $? -ne 0 ]; then
            return 1
        fi
        "./$submission_file.out" > "$parentdir/${id}_output.txt"
        rm "$submission_file.out"
        # mv "$parentdir" "checked/"
    elif [ "$extension" = "sh" ]; then
        bash "$submission_file" > "$parentdir/${id}_output.txt"
        if [ $? -ne 0 ]; then
            return 1
        fi
    elif [ "$extension" = "py" ]; then
        python "$submission_file" > "$parentdir/${id}_output.txt"
        if [ $? -ne 0 ]; then
            return 1
        fi
    fi
}

# $1 = output_file
match_output()
{
    local diff=0
    local output_file=$1
    # echo "ID: $id, output_file: $output_file" >> log.txt
    if [ ! -f "$output_file" ]; then
        return -1
    fi

    for line in "${expected_output_content[@]}"; do
        local isfound=$(cat "$output_file" | grep -c "^${line}$")
        # echo "$line -> $isfound" >> log.txt
        if [ $isfound -eq 0 ]; then
            diff=$((diff+1))
        fi
    done
    # echo "$diff" >> log.txt
    return $diff
}

for ((id=start_id; id<=end_id; id++)); do
    copy_status=$(has_copied $id)
    was_archived="false"
    has_i4="false"
    # echo "$copy_status"
    marks=$total_marks
    deducted=0
    total=0
    remarks=""
    # submission_file="$working_directory/$id"
    submission_file=$(find "$working_directory" -name "$id*" -type f)

    if [ -z "$submission_file" ]; then #either a directory or not found

        submission_file=$(find "$working_directory" -name "$id*" -type d)

        if [ -z "$submission_file" ]; then #not found
            echo "$id,0,0,$total,submission missing" >> marks.csv
            continue
        else
            mv "$submission_file" temporary/  #is a directory
        fi

    else
        zrt=$(file "$submission_file" | grep -c 'archive')
        if [ $zrt -eq 1 ]; then
            unarchive "$submission_file" "$id"
            if [ $? -eq 1 ]; then
                continue
            fi
            was_archived="true"
            dirname=$(ls "temporary/")
            if [ "$dirname" != "$id" ]; then
                has_i4="true"
            fi
        else
            mkdir -p "temporary/$id"
            cp "$submission_file" "temporary/$id/"
        fi
    fi

    submission_file=$(find "temporary/" -name "$id*" -type f)
    directory=$(ls "temporary/")
    if [ -z "$submission_file" ]; then #case 3
        echo "$id,0,10,-10,issue case #3" >> marks.csv
        mv "temporary/$directory" issues/
        continue
    fi
    # echo "$submission_file" "$directory"
    generate_output "$submission_file" "$id"
    compilation_status=$?
    if [ $compilation_status -eq 1 ]; then
        rm -rf "temporary/$directory"
        echo "$id,0,0,0,compilation error" >> marks.csv
        continue
    elif [ $compilation_status -eq 2 ]; then
        mv "temporary/$directory" "issues/"
        echo "$id,0,10,-10,issue case #3" >> marks.csv
        continue
    fi

    output_file=$(find "temporary/" -name "${id}_output.txt" -type f)
    # diff_cnt=$(match_output "$output_file")
    match_output "$output_file"
    diff_cnt=$?
    mv "temporary/$directory" "checked/"
    # echo "$diff_cnt"

    if [[ $diff_cnt -gt 0 ]]; then
        marks=$((total_marks-penalty_unmatched_output*diff_cnt))
    fi

    if [[ $is_archived != $was_archived ]]; then
        deducted=$((deducted+penalty_guidelines))
        remarks="issue case #1" 
    fi

    if [ "$has_i4" = "true" ]; then
        deducted=$((deducted+penalty_guidelines))
        remarks="$remarks issue case #4"
    fi

    if [ "$copy_status" = "true" ]; then
        pm_cnt=$(awk "BEGIN {print ($plagiarism_penalty*$total_marks)/100}") 
        remarks="$remarks plagiarism detected"
        echo "$id,$marks,$deducted,-$pm_cnt,$remarks" >> marks.csv
        continue
    fi

    total=$((marks-deducted))
    echo "$id,$marks,$deducted,$total,$remarks" >> marks.csv

done

rm -rf temporary