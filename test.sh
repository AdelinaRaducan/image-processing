#!/bin/bash

function init {
    mkdir -p output
    gcc -std=c99 -Wall -Werror -Wextra -o ./diff_bmp diff_bmp.c

    tests_nr=5
    task1_points=$((30/tests_nr))  
    task2_points=$task1_points
    task3_points=$((40/tests_nr))

    make build
    if [ $? -ne 0 ]; then
        echo "Makefile failed!"
        exit 1
    fi

    if [ $# == 0 ]; then
        s_test=1
        f_test=$tests_nr
    elif [ "$1" == "all" ]; then
        s_test=1
        f_test=$tests_nr
    elif [ $1 -ge 1 ] && [ $1 -le $tests_nr ]; then
        s_test=$1
        f_test=$1
    else
        echo "Test $1 doesn't  exist!"
        exit 1
    fi

    tolerance=5
    total=0
}

function prepare_test {
    cp "./input/input$1" ./input.txt
    cp "./input/input${1}.bmp" ./input.bmp

    echo "TEST $1:"
}

function clean_test {
    rm -rf ./input.txt
    rm -rf ./input.bmp
    rm -rf output_*
    echo ""
}

function check_exit_status {
    case $1 in
        1)
            echo "    *Error while reading the file header part of the image."
            echo "    *Make sure your output file is correctly formated."
        ;;
        2)
            echo "    *Error while reading the info header part of the image."
            echo "    *Make sure your output file is correctly formated."
        ;;
        3)
            echo "    *The FILEMARKER1 field is not correctly set in your output image."
            echo "    *It should be set to the ASCII value of 'B'."
        ;;
        4)
            echo "    *The FILEMARKER2 field is not correctly set in your output image."
            echo "    *It should be set to the ASCII value of 'M'."
        ;;
        5)
            echo "    *The file size is not correctly set in your bmp's file header (bfSize)."
            echo "    *It should be the sum of the bitmap size and the size of the headers"
        ;;
        6)
            echo "    *The imageDataOffset is not correctly set in your bmp's file header."
            echo "    *Its value should be the constant 54."
        ;;
        7)
            echo "    *The info header's size is not correctly set in your bmp's info header."
            echo "    *Its value should be the constant 40."
        ;;
        8)
            echo "    *The width of the bitmap is not correctly set in your bmp's info header."
            echo "    *Make sure it is set to the actual width of the bitmap (excluding the padding)."
        ;;
        9)
            echo "    *The height of the bitmap is not correctly set in your bmp's info header."
        ;;
        10)
            echo "    *The size of the bitmap is not correctly set in your bmp's info header."
            echo "    *Make sure you take into consideration the padding, if it is the case."
        ;;
        11)
            echo "    *The biXPelsPerMeter field of your bmp's info header is not set to 0."
        ;;
        12)
            echo "    *The biYPelsPerMeter field of your bmp's info header is not set to 0."
        ;;
        13)
            echo "    *Some pixels from your' bmp's bitmap differ from the ref."
            echo "    *Make sure you use the double data type for P."
            echo "    *Make sure that when blurring a pixel, the values for the neighbouring pixels"
            echo "     (especially the left and upper ones) are the ones from the previous"
            echo "      iteration, not the current one."
            echo "    *Make sure that when croping a cluster, you actually crop the smallest"
            echo "     rectangle containing that cluster."
        ;;
        14)
            echo "    *Make sure that any padding bytes are explicitly set to zero."
        ;;
        15)
            echo "    *Your bmp's bitmap and the ref differ."
        ;;

    esac
}

# Check task 1
function check_task1 {
    if [ -f ./output.txt ]; then
        mv -f ./output.txt ./output/output$i
        diff -w ./output/output$i ./ref/output$i &>/dev/null
        if [ $? -eq 0 ]; then
            total=$((total+task1_points))
            echo "  Task 1: OK!"
        else
            echo "  Task 1: Wrong!"
        fi
    else
        echo "  Task 1: File output.txt is missing"
    fi
}

# Check task 2
function check_task2 {
    if [ -f ./output_blur.bmp ]; then
        echo "  Task 2:"
        mv -f ./output_blur.bmp ./output/output${i}_blur.bmp
        ./diff_bmp "./output/output${i}_blur.bmp" "./ref/output${i}_blur.bmp" "$tolerance"
        exit_status=$?
        if [ $exit_status -eq 0 ]; then
            total=$((total+task2_points))
            echo "    Result: OK!"
        else
            check_exit_status $exit_status
            echo "    Result: Wrong!"
        fi
    else
        echo "  Task 2: File output_blur.bmp is missing"
    fi
}

# Check task 3
function check_task3 {
    echo "  Task 3:"
    clusters_nr=$(ls ./ref | grep output$i | grep crop | sed -e "s/output${i}_crop\([0-9]*\)\.bmp/\1/" | sort -n | tail -n 1)
    for ((j=1; j<=$clusters_nr; j++)); do
        checked[j]=0
    done

    total_done=0
    for ((j=1; j<=$clusters_nr; j++)); do
        if [ ! -f ./output_crop${j}.bmp ]; then
            echo "    File output_crop${j}.bmp is missing"
            continue
        fi

        hexdump ./output_crop${j}.bmp >./out_dump
        for ((k=1; k<=$clusters_nr; ++k)); do
            hexdump ./ref/output${i}_crop${k}.bmp >./ref_dump
            diff -w  ./out_dump ./ref_dump &>/dev/null
            if [ $? -eq 0 ]; then
                break
            fi
        done

        if [ $k -le $clusters_nr ]; then
            if [ ${checked[k]} -eq 0 ]; then
                echo "    *Cluster number $j has been correctly cropped. It has been matched to ref $k."
                mv -f ./output_crop${j}.bmp ./output/output${i}_crop${j}.bmp
                checked[k]=1
                total_done=$((total_done+1))
            else
                continue
            fi
        else
            echo "    Cluster number $j has not been correctly cropped"
        fi

        rm -rf ./out_dump ./ref_dump
    done
    task3_total=$(((total_done * task3_points)/clusters_nr))
    if [ $total_done -eq $clusters_nr ]; then
        echo "    Result: OK!"
        total=$((total+task3_total))
    else
        if [ $total_done -eq 0 ]; then
            echo "    Result: Wrong!"
        else
            echo "    Result: $total_done from $clusters_nr clusters correctly cropped!"
            total=$((total+task3_total))
        fi
    fi
}

function clean {
    make clean
    # rm -rf output
    rm -rf ./diff_bmp
}

function print_result {
    echo -e "Total: $total points\n"
}


# Initialize variables
init "$@"

# Iterate through each test
for ((i=$s_test; i<=$f_test; i++)); do
    # Make initializations required before running a test
    prepare_test "$i"

    # Run program
    ./tema3
    if [ $? -ne 0 ]; then
        echo "Program ended abnormally"
        clean_test
        break
    fi

    # Check tasks
    check_task1
    check_task2
    check_task3

    # Clean
    clean_test
done

print_result
clean
