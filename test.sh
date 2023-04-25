#!/bin/bash

# Test function definition
function test() {
    local description="$1"
    local command="$2"
    local expected_status="$3"

    echo "Testing: $description"
    status_code=$(eval "$command")
    if [ "$status_code" -eq "$expected_status" ]; then
        echo "PASSED"
    else
        echo "FAILED: Expected $expected_status, got $status_code"
    fi
}

# Cache test cases
test "GET http://netsys.cs.colorado.edu/index.html failed" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' --cache './cache' 'http://netsys.cs.colorado.edu/index.html'" "000"
test "GET http://netsys.cs.colorado.edu recursive failed" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' --cache './cache' --recursive 'http://netsys.cs.colorado.edu'" "000"
test "2 GET http://netsys.cs.colorado.edu 10% parallel match" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' --cache './cache' --parallel 2 --recursive 'http://netsys.cs.colorado.edu' -X GET" "000"

# Bad request test cases
test "HELO http://netsys.cs.colorado.edu/index.html doesn't return 4xx" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' -X HELO 'http://netsys.cs.colorado.edu/index.html'" "400"
test "PUT http://netsys.cs.colorado.edu/newfile.dat doesn't return 4xx" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' -X PUT 'http://netsys.cs.colorado.edu/newfile.dat'" "400"
test "GET http://bar.cs.colorado.edu/index.html doesn't return 4xx" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' 'http://bar.cs.colorado.edu/index.html'" "400"
test "GET http://netsys.cs.colorado.edu:88/index.html doesn't return 4xx" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' 'http://netsys.cs.colorado.edu:88/index.html'" "400"
test "GET http://netsys.cs.colorado.edu/nosuchfile.html doesn't return 4xx" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' 'http://netsys.cs.colorado.edu/nosuchfile.html'" "400"

# Blocked site test cases
test "GET http://www.google.com/index.html blocked doesn't return 403" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' 'http://www.google.com/index.html'" "403"
test "GET http://www.facebook.com blocked doesn't return 403" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' 'http://www.facebook.com'" "403"
test "GET http://www.facebook.com:8080 blocked doesn't return 403" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' 'http://www.facebook.com:8080'" "403"
test "GET http://23.237.16.149 blocked doesn't return 403" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' 'http://23.237.16.149'" "403"
test "GET http://23.237.16.149:80 blocked doesn't return 403" "curl -s -o /dev/null -w '%{http_code}' -x 'http://localhost:$port' 'http://23.237.16.149:80'" "403"