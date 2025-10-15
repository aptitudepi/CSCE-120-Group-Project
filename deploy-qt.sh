#!/bin/bash
# deploy-qt.sh - Deployment script for C++/Qt Hyperlocal Weather System

echo "🌤️  Deploying C++/Qt Hyperlocal Weather Forecasting System"
echo "============================================================"

# Check if Docker and Docker Compose are installed
if ! command -v docker &> /dev/null; then
    echo "❌ Docker is not installed. Please install Docker first."
    exit 1
fi

if ! command -v docker-compose &> /dev/null; then
    echo "❌ Docker Compose is not installed. Please install Docker Compose first."
    exit 1
fi

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "❌ Build directory not found. Please run ./build.sh first."
    exit 1
fi

# Create environment file if it doesn't exist
if [ ! -f .env ]; then
    echo "📝 Creating .env file from template..."
    cp .env.template .env
    echo "⚠️  Please edit .env file with your API keys and configuration"
    echo "   Required: PIRATE_WEATHER_API_KEY"
    read -p "Press Enter to continue after editing .env file..."
fi

# Create Dockerfile.qt from template
echo "🐳 Creating Dockerfile for Qt services..."
cp Dockerfile.qt.template Dockerfile.qt

# Build Docker images
echo "🔨 Building Docker images..."
docker-compose -f docker-compose-qt.yml build

# Start infrastructure services first
echo "🗄️  Starting infrastructure services..."
docker-compose -f docker-compose-qt.yml up -d postgres redis

# Wait for infrastructure to be ready
echo "⏳ Waiting for infrastructure services..."
sleep 15

# Start all services
echo "🚀 Starting all microservices..."
docker-compose -f docker-compose-qt.yml up -d

# Wait for services to be ready
echo "⏳ Waiting for services to start..."
sleep 20

# Check service health
echo "🏥 Checking service health..."
services=("api-gateway:8000" "weather-data:8001" "ml-service:8002" "location-service:8003" "alert-service:8004" "data-processing:8005" "database-service:8006")

all_healthy=true
for service in "${services[@]}"; do
    IFS=':' read -r name port <<< "$service"
    if curl -s http://localhost:$port/health > /dev/null; then
        echo "✅ $name service is healthy"
    else
        echo "❌ $name service is not responding"
        all_healthy=false
    fi
done

echo ""
if [ "$all_healthy" = true ]; then
    echo "🎉 All services deployed successfully!"
else
    echo "⚠️  Some services are not healthy. Check logs with:"
    echo "   docker-compose -f docker-compose-qt.yml logs"
fi

echo ""
echo "📍 Services available at:"
echo "   - API Gateway: http://localhost:8000"
echo "   - Weather Data Service: http://localhost:8001"
echo "   - ML Service: http://localhost:8002"
echo "   - Location Service: http://localhost:8003"
echo "   - Alert Service: http://localhost:8004"
echo "   - Data Processing Service: http://localhost:8005"
echo "   - Database Service: http://localhost:8006"
echo ""
echo "🖥️  To run the Qt frontend application:"
echo "   cd build/frontend"
echo "   ./HyperlocalWeatherApp"
echo ""
echo "🧪 To test the API:"
echo "   ./test-api-qt.sh"
echo ""
echo "📊 To view logs:"
echo "   docker-compose -f docker-compose-qt.yml logs -f"
echo ""
echo "🛑 To stop services:"
echo "   docker-compose -f docker-compose-qt.yml down"
