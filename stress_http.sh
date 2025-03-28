#!/bin/bash

# Check if the required arguments are provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <number_of_requests> <url>"
    echo "Example: $0 10 http://example.com"
    exit 1
fi

n=$1
url=$2

# Function to send a single request
send_request() {
    local start_time=$(date +%s.%N)
    # Using curl with silent mode (remove -s if you want to see output)
    curl -ik  --compressed "$url" 
    local end_time=$(date +%s.%N)
    local duration=$(echo "$end_time - $start_time" | bc)
    echo "Request completed in $duration seconds"
}

echo "Sending $n simultaneous requests to $url"

# Create and send all requests in background
for ((i=1; i<=$n; i++)); do
    send_request &
    pids[$i]=$!
done

# Wait for all requests to complete
for pid in ${pids[*]}; do
    wait $pid
done

echo "All $n requests completed"