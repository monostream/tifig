#!/bin/bash

echo "Preparing env..."
virtualenv -q venv
source venv/bin/activate
pip install -q -r requirements.txt

python tests.py
deactivate