#!/bin/bash

if [ "$#" -ne 1 ]; then
    echo "Usage: bash $0 <character>"
    exit 1
fi

character=$1
counter=0

function is_valid() {
    local line=$1

    # Check if the line contains the specified character                  
    [[ $line == *"$character"* ]] &&
    # Check if the line starts with an uppercase letter
    [[ $line =~ ^[A-Z] ]] &&        
    # Check if the line contains only alphanumeric characters, spaces, and specified punctuation         
    [[ $line =~ [a-zA-Z0-9\ \!\?\.]+$ ]] &&    
    # Check if the line ends with a period, exclamation mark, or question mark   
    [[ $line =~ (\.|\!|\?)$ ]] &&     
     # Check if the line does not contain the conjunction ", și"             
    [[ ! $line =~ ,\ și ]]                      
}

while IFS= read -r line || [[ -n "$line" ]]; do
    if is_valid "$line"; then
        ((counter++))
    fi
done

echo "$counter"