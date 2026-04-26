#!/bin/bash
# Sun Apr 26 11:59:04 AM PDT 2026
# By vkardon

# Stop and remove all containers and networks
docker compose down --volumes --remove-orphans

# Remove your specific project images
docker rmi hashsrv_server hashsrv_client

# Remove the base Ubuntu image
docker rmi ubuntu:24.04

# Final wipe of all cached build layers
docker system prune -a --volumes -f


