#!/bin/bash

# License Consistency Checker
# This script verifies that the license information in README.md matches the actual LICENSE file

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "🔍 Checking license consistency..."

# Check if LICENSE file exists
if [ ! -f "LICENSE" ]; then
    echo -e "${RED}❌ LICENSE file not found!${NC}"
    exit 1
fi

# Check if README.md exists
if [ ! -f "README.md" ]; then
    echo -e "${RED}❌ README.md file not found!${NC}"
    exit 1
fi

# Detect license type from LICENSE file
if grep -q "GNU GENERAL PUBLIC LICENSE" LICENSE && grep -q "Version 3" LICENSE; then
    ACTUAL_LICENSE="GPL-3.0"
    EXPECTED_BADGE="GPL--3.0"
    EXPECTED_TEXT="GNU通用公共许可证第3版(GPL v3)"
elif grep -q "MIT License" LICENSE; then
    ACTUAL_LICENSE="MIT"
    EXPECTED_BADGE="MIT"
    EXPECTED_TEXT="MIT License"
elif grep -q "Apache License" LICENSE && grep -q "Version 2.0" LICENSE; then
    ACTUAL_LICENSE="Apache-2.0"
    EXPECTED_BADGE="Apache--2.0"
    EXPECTED_TEXT="Apache License 2.0"
else
    echo -e "${YELLOW}⚠️  Unknown license type in LICENSE file${NC}"
    ACTUAL_LICENSE="UNKNOWN"
fi

echo "📋 Detected license: $ACTUAL_LICENSE"

# Check README.md badge
if grep -q "license-${EXPECTED_BADGE}" README.md; then
    echo -e "${GREEN}✅ License badge is correct${NC}"
    BADGE_OK=true
else
    echo -e "${RED}❌ License badge is incorrect or missing${NC}"
    echo "   Expected: license-${EXPECTED_BADGE}"
    echo "   Found in README.md:"
    grep "license-" README.md || echo "   No license badge found"
    BADGE_OK=false
fi

# Check README.md license text
if grep -q "$EXPECTED_TEXT" README.md; then
    echo -e "${GREEN}✅ License text is correct${NC}"
    TEXT_OK=true
else
    echo -e "${RED}❌ License text is incorrect or missing${NC}"
    echo "   Expected: $EXPECTED_TEXT"
    echo "   Found in README.md:"
    grep -A2 -B2 "许可证\|License" README.md || echo "   No license text found"
    TEXT_OK=false
fi

# Summary
echo ""
echo "📊 License Consistency Check Summary:"
echo "   LICENSE file: $ACTUAL_LICENSE"
echo "   Badge status: $([ "$BADGE_OK" = true ] && echo "✅ OK" || echo "❌ FAILED")"
echo "   Text status:  $([ "$TEXT_OK" = true ] && echo "✅ OK" || echo "❌ FAILED")"

if [ "$BADGE_OK" = true ] && [ "$TEXT_OK" = true ]; then
    echo -e "${GREEN}🎉 All license information is consistent!${NC}"
    exit 0
else
    echo -e "${RED}💥 License inconsistency detected!${NC}"
    echo ""
    echo "🔧 To fix this issue:"
    echo "   1. Update the license badge in README.md to: license-${EXPECTED_BADGE}"
    echo "   2. Update the license text in README.md to reference: $EXPECTED_TEXT"
    echo "   3. Run this script again to verify the fix"
    exit 1
fi 