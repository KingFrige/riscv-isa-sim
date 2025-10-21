#!/bin/bash

# Install Dependencies
#  run: sudo xargs apt-get install -y < .github/workflows/apt-packages.txt

for commit in $(git rev-list origin/master..HEAD | tac); do
  git checkout $commit
  echo "Checking commit $commit"
  ci-tests/build-spike
  ci-tests/test-spike
done
