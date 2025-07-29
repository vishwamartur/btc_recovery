#!/bin/bash

# AWS EC2 Deployment Script for Bitcoin Wallet Recovery

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
AWS_REGION=${AWS_REGION:-us-east-1}
INSTANCE_TYPE=${INSTANCE_TYPE:-c5.xlarge}
AMI_ID=${AMI_ID:-ami-0abcdef1234567890}  # Update with actual Ubuntu AMI
KEY_PAIR=${KEY_PAIR:-btc-recovery-key}
SECURITY_GROUP=${SECURITY_GROUP:-btc-recovery-sg}
SUBNET_ID=${SUBNET_ID:-}
INSTANCE_COUNT=${INSTANCE_COUNT:-1}
CLUSTER_MODE=${CLUSTER_MODE:-false}

print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check AWS CLI
check_aws_cli() {
    print_status "Checking AWS CLI..."
    
    if ! command -v aws >/dev/null 2>&1; then
        print_error "AWS CLI is required but not installed."
        print_error "Install with: pip install awscli"
        exit 1
    fi
    
    # Check AWS credentials
    if ! aws sts get-caller-identity >/dev/null 2>&1; then
        print_error "AWS credentials not configured."
        print_error "Run: aws configure"
        exit 1
    fi
    
    print_status "AWS CLI check completed"
}

# Create security group
create_security_group() {
    print_status "Creating security group..."
    
    # Check if security group exists
    if aws ec2 describe-security-groups --group-names "$SECURITY_GROUP" --region "$AWS_REGION" >/dev/null 2>&1; then
        print_warning "Security group $SECURITY_GROUP already exists"
        return
    fi
    
    # Create security group
    SECURITY_GROUP_ID=$(aws ec2 create-security-group \
        --group-name "$SECURITY_GROUP" \
        --description "Bitcoin Wallet Recovery Security Group" \
        --region "$AWS_REGION" \
        --query 'GroupId' --output text)
    
    # Add SSH access
    aws ec2 authorize-security-group-ingress \
        --group-id "$SECURITY_GROUP_ID" \
        --protocol tcp \
        --port 22 \
        --cidr 0.0.0.0/0 \
        --region "$AWS_REGION"
    
    # Add cluster communication ports
    if [[ "$CLUSTER_MODE" == "true" ]]; then
        aws ec2 authorize-security-group-ingress \
            --group-id "$SECURITY_GROUP_ID" \
            --protocol tcp \
            --port 8080 \
            --source-group "$SECURITY_GROUP_ID" \
            --region "$AWS_REGION"
        
        aws ec2 authorize-security-group-ingress \
            --group-id "$SECURITY_GROUP_ID" \
            --protocol tcp \
            --port 9090 \
            --source-group "$SECURITY_GROUP_ID" \
            --region "$AWS_REGION"
    fi
    
    print_status "Security group created: $SECURITY_GROUP_ID"
}

# Create key pair
create_key_pair() {
    print_status "Checking key pair..."
    
    if aws ec2 describe-key-pairs --key-names "$KEY_PAIR" --region "$AWS_REGION" >/dev/null 2>&1; then
        print_warning "Key pair $KEY_PAIR already exists"
        return
    fi
    
    print_status "Creating key pair..."
    aws ec2 create-key-pair \
        --key-name "$KEY_PAIR" \
        --region "$AWS_REGION" \
        --query 'KeyMaterial' --output text > "${KEY_PAIR}.pem"
    
    chmod 400 "${KEY_PAIR}.pem"
    print_status "Key pair created: ${KEY_PAIR}.pem"
}

# Generate user data script
generate_user_data() {
    cat > user_data.sh << 'EOF'
#!/bin/bash

# Update system
apt-get update -y
apt-get upgrade -y

# Install dependencies
apt-get install -y \
    build-essential \
    cmake \
    git \
    libssl-dev \
    pkg-config \
    wget \
    curl \
    htop \
    screen

# Install CUDA (optional)
if lspci | grep -i nvidia > /dev/null; then
    wget https://developer.download.nvidia.com/compute/cuda/repos/ubuntu2004/x86_64/cuda-ubuntu2004.pin
    mv cuda-ubuntu2004.pin /etc/apt/preferences.d/cuda-repository-pin-600
    wget https://developer.download.nvidia.com/compute/cuda/11.8.0/local_installers/cuda-repo-ubuntu2004-11-8-local_11.8.0-520.61.05-1_amd64.deb
    dpkg -i cuda-repo-ubuntu2004-11-8-local_11.8.0-520.61.05-1_amd64.deb
    cp /var/cuda-repo-ubuntu2004-11-8-local/cuda-*-keyring.gpg /usr/share/keyrings/
    apt-get update
    apt-get -y install cuda
fi

# Create application directory
mkdir -p /opt/btc-recovery
cd /opt/btc-recovery

# Clone repository (replace with actual repository)
# git clone https://github.com/your-repo/btc-recovery.git .

# Build application
# ./scripts/build.sh

# Create systemd service
cat > /etc/systemd/system/btc-recovery.service << 'SERVICE_EOF'
[Unit]
Description=Bitcoin Wallet Recovery Service
After=network.target

[Service]
Type=simple
User=ubuntu
WorkingDirectory=/opt/btc-recovery
ExecStart=/opt/btc-recovery/build/btc-recovery --config /opt/btc-recovery/config/recovery.yaml
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
SERVICE_EOF

# Enable service
systemctl daemon-reload
systemctl enable btc-recovery

# Set up monitoring
apt-get install -y prometheus-node-exporter
systemctl enable prometheus-node-exporter
systemctl start prometheus-node-exporter

# Configure log rotation
cat > /etc/logrotate.d/btc-recovery << 'LOGROTATE_EOF'
/opt/btc-recovery/logs/*.log {
    daily
    rotate 7
    compress
    delaycompress
    missingok
    notifempty
    create 644 ubuntu ubuntu
}
LOGROTATE_EOF

echo "Setup completed" > /tmp/setup_complete
EOF
}

# Launch instances
launch_instances() {
    print_status "Launching $INSTANCE_COUNT EC2 instances..."
    
    generate_user_data
    
    INSTANCE_IDS=()
    
    for ((i=0; i<INSTANCE_COUNT; i++)); do
        print_status "Launching instance $((i+1))/$INSTANCE_COUNT..."
        
        INSTANCE_ID=$(aws ec2 run-instances \
            --image-id "$AMI_ID" \
            --count 1 \
            --instance-type "$INSTANCE_TYPE" \
            --key-name "$KEY_PAIR" \
            --security-groups "$SECURITY_GROUP" \
            --user-data file://user_data.sh \
            --region "$AWS_REGION" \
            --tag-specifications "ResourceType=instance,Tags=[{Key=Name,Value=btc-recovery-node-$i},{Key=Project,Value=btc-recovery}]" \
            --query 'Instances[0].InstanceId' --output text)
        
        INSTANCE_IDS+=("$INSTANCE_ID")
        print_status "Instance launched: $INSTANCE_ID"
    done
    
    # Wait for instances to be running
    print_status "Waiting for instances to be running..."
    for INSTANCE_ID in "${INSTANCE_IDS[@]}"; do
        aws ec2 wait instance-running --instance-ids "$INSTANCE_ID" --region "$AWS_REGION"
        print_status "Instance $INSTANCE_ID is running"
    done
    
    # Get instance details
    print_status "Instance details:"
    for INSTANCE_ID in "${INSTANCE_IDS[@]}"; do
        PUBLIC_IP=$(aws ec2 describe-instances \
            --instance-ids "$INSTANCE_ID" \
            --region "$AWS_REGION" \
            --query 'Reservations[0].Instances[0].PublicIpAddress' --output text)
        
        echo "  Instance: $INSTANCE_ID"
        echo "  Public IP: $PUBLIC_IP"
        echo "  SSH: ssh -i ${KEY_PAIR}.pem ubuntu@$PUBLIC_IP"
        echo
    done
    
    # Clean up
    rm -f user_data.sh
}

# Terminate instances
terminate_instances() {
    print_status "Finding instances to terminate..."
    
    INSTANCE_IDS=$(aws ec2 describe-instances \
        --filters "Name=tag:Project,Values=btc-recovery" "Name=instance-state-name,Values=running,pending,stopping,stopped" \
        --region "$AWS_REGION" \
        --query 'Reservations[].Instances[].InstanceId' --output text)
    
    if [[ -z "$INSTANCE_IDS" ]]; then
        print_warning "No instances found to terminate"
        return
    fi
    
    print_status "Terminating instances: $INSTANCE_IDS"
    aws ec2 terminate-instances --instance-ids $INSTANCE_IDS --region "$AWS_REGION"
    
    print_status "Waiting for instances to terminate..."
    aws ec2 wait instance-terminated --instance-ids $INSTANCE_IDS --region "$AWS_REGION"
    print_status "All instances terminated"
}

# Main execution
main() {
    case "$1" in
        deploy)
            check_aws_cli
            create_key_pair
            create_security_group
            launch_instances
            ;;
        terminate)
            check_aws_cli
            terminate_instances
            ;;
        status)
            check_aws_cli
            aws ec2 describe-instances \
                --filters "Name=tag:Project,Values=btc-recovery" \
                --region "$AWS_REGION" \
                --query 'Reservations[].Instances[].[InstanceId,State.Name,PublicIpAddress,InstanceType]' \
                --output table
            ;;
        help|--help|-h)
            echo "Usage: $0 [deploy|terminate|status|help]"
            echo
            echo "Commands:"
            echo "  deploy     - Deploy instances to AWS EC2"
            echo "  terminate  - Terminate all project instances"
            echo "  status     - Show status of all instances"
            echo "  help       - Show this help message"
            echo
            echo "Environment variables:"
            echo "  AWS_REGION      - AWS region [default: us-east-1]"
            echo "  INSTANCE_TYPE   - EC2 instance type [default: c5.xlarge]"
            echo "  AMI_ID          - AMI ID to use [required]"
            echo "  KEY_PAIR        - Key pair name [default: btc-recovery-key]"
            echo "  SECURITY_GROUP  - Security group name [default: btc-recovery-sg]"
            echo "  INSTANCE_COUNT  - Number of instances [default: 1]"
            echo "  CLUSTER_MODE    - Enable cluster mode [default: false]"
            ;;
        *)
            print_error "Unknown command: $1"
            echo "Use '$0 help' for usage information"
            exit 1
            ;;
    esac
}

main "$@"
