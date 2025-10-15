#!/bin/bash
# test-api-qt.sh - API Testing Script for C++/Qt Services

echo "🧪 Testing C++/Qt Hyperlocal Weather Forecasting API"
echo "===================================================="

API_BASE="http://localhost:8000"
USERNAME="demo"
PASSWORD="demo123"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to test HTTP endpoint
test_endpoint() {
    local method=$1
    local endpoint=$2
    local data=$3
    local expected_status=$4
    local description=$5

    echo -e "${BLUE}Testing:${NC} $description"

    if [ "$method" == "GET" ]; then
        response=$(curl -s -w "\n%{http_code}" "$API_BASE$endpoint" -H "Authorization: Bearer $TOKEN")
    else
        response=$(curl -s -w "\n%{http_code}" -X "$method" "$API_BASE$endpoint" \
            -H "Content-Type: application/json" \
            -H "Authorization: Bearer $TOKEN" \
            -d "$data")
    fi

    status_code=$(echo "$response" | tail -n1)
    body=$(echo "$response" | head -n -1)

    if [ "$status_code" -eq "$expected_status" ]; then
        echo -e "${GREEN}✅ PASS${NC} - Status: $status_code"
    else
        echo -e "${RED}❌ FAIL${NC} - Expected: $expected_status, Got: $status_code"
        echo "Response: $body"
    fi
    echo
}

# Test service health first
echo -e "${YELLOW}🏥 Testing Service Health${NC}"
echo "=========================="

services=("api-gateway:8000" "weather-data:8001" "ml-service:8002" "location-service:8003" "alert-service:8004" "data-processing:8005" "database-service:8006")

healthy_count=0
for service in "${services[@]}"; do
    IFS=':' read -r name port <<< "$service"
    if curl -s http://localhost:$port/health > /dev/null; then
        echo -e "${GREEN}✅ $name${NC} - Healthy"
        ((healthy_count++))
    else
        echo -e "${RED}❌ $name${NC} - Not responding"
    fi
done

echo
echo "Health Summary: $healthy_count/${#services[@]} services healthy"
echo

if [ $healthy_count -ne ${#services[@]} ]; then
    echo -e "${RED}⚠️  Not all services are healthy. Some tests may fail.${NC}"
    echo
fi

# Test Authentication
echo -e "${YELLOW}🔐 Testing Authentication${NC}"
echo "========================"

auth_response=$(curl -s -w "\n%{http_code}" -X POST "$API_BASE/auth/token" \
    -H "Content-Type: application/x-www-form-urlencoded" \
    -d "username=$USERNAME&password=$PASSWORD")

auth_status=$(echo "$auth_response" | tail -n1)
auth_body=$(echo "$auth_response" | head -n -1)

if [ "$auth_status" -eq 200 ]; then
    echo -e "${GREEN}✅ Authentication successful${NC}"
    TOKEN=$(echo "$auth_body" | grep -o '"access_token":"[^"]*' | cut -d'"' -f4)
    echo "Token obtained: ${TOKEN:0:20}..."
else
    echo -e "${RED}❌ Authentication failed${NC}"
    echo "Response: $auth_body"
    exit 1
fi
echo

# Test Weather Endpoints
echo -e "${YELLOW}🌤️  Testing Weather Endpoints${NC}"
echo "============================="

# College Station, TX coordinates
LAT=30.6280
LON=-96.3344

test_endpoint "GET" "/weather/current/$LAT/$LON" "" 200 "Get current weather"
test_endpoint "GET" "/weather/forecast/$LAT/$LON" "" 200 "Get weather forecast"

# Test Location Endpoints
echo -e "${YELLOW}🗺️  Testing Location Endpoints${NC}"
echo "=============================="

ADDRESS="College%20Station%2C%20TX"
test_endpoint "GET" "/location/geocode/$ADDRESS" "" 200 "Geocode address"

# Test Alert Subscription
echo -e "${YELLOW}🚨 Testing Alert Endpoints${NC}"
echo "=========================="

ALERT_DATA='{
    "user_id": "test_user",
    "location_name": "Test Location",
    "latitude": 30.6280,
    "longitude": -96.3344,
    "alert_types": ["precipitation", "wind"],
    "threshold_config": {
        "precipitation_threshold": 1.0,
        "wind_threshold": 10.0
    },
    "notification_methods": ["push"]
}'

test_endpoint "POST" "/alerts/subscribe" "$ALERT_DATA" 200 "Subscribe to alerts"

# Test Summary
echo -e "${YELLOW}📊 Test Summary${NC}"
echo "=============="
echo "All critical API endpoints have been tested."
echo
echo "🔗 API Documentation: http://localhost:8000"
echo "📊 Service Status: All services should be responding"
echo
echo -e "${GREEN}🎉 API Testing Complete!${NC}"
echo
echo "Next steps:"
echo "1. Run the Qt frontend: cd build/frontend && ./HyperlocalWeatherApp"
echo "2. Test with demo credentials: demo / demo123"
echo "3. Try geocoding and weather forecasts"
