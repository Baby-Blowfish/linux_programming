#!/bin/bash

TEMPLATE="$HOME/.project_templates/make_project_template.tar.gz"
PROJECT_NAME=$1

# 색상 정의
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# 인자 없을 때
if [ -z "$PROJECT_NAME" ]; then
    echo -e "${GREEN}사용법: new_project 프로젝트이름${NC}"
    exit 1
fi

# 템플릿 파일이 없을 때
if [ ! -f "$TEMPLATE" ]; then
    echo "❌ 템플릿 파일이 존재하지 않습니다: $TEMPLATE"
    exit 1
fi

# 현재 위치에 압축 해제
tar -xzf "$TEMPLATE"
mv my_template_project "$PROJECT_NAME"

echo -e "${GREEN}✅ 새 프로젝트 생성 완료: $PROJECT_NAME${NC}"

