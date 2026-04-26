#!/bin/bash
# Sun Apr 26 12:04:51 PM PDT 2026
# By vkardon

# Exit immediately if a command exits with a non-zero status
set -e

cd ..

echo "### Cleaning up old containers and images..."
docker compose down --volumes --remove-orphans

echo "### Remove project images to force a fresh build..."
docker rmi hashsrv_server hashsrv_client ubuntu:24.04 || true
docker system prune -a --volumes -f

echo "### Building images from scratch..."
docker compose build --no-cache

echo "### Reclaiming disk space from build layers..."
docker image prune -f

echo "### Running Unit Tests: Session Logic..."
docker run --rm --entrypoint ./session_test hashsrv_server

echo "### Running Unit Tests: Server Logic..."
docker run --rm --entrypoint ./server_test hashsrv_server

echo "### Running System Integration Test..."
echo "    Note: Server will shut down automatically when client finishes."
docker compose up --abort-on-container-exit --exit-code-from client

echo "### Done! All tests passed!"

