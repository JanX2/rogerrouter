#!/bin/bash
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory out --branch-coverage --function-coverage --legend --highlight