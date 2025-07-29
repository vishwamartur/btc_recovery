# Contributing to Bitcoin Wallet Recovery System

Thank you for your interest in contributing to the Bitcoin Wallet Recovery System! This document provides guidelines for contributing to the project.

## Code of Conduct

By participating in this project, you agree to abide by our code of conduct:

- **Legitimate Use Only**: All contributions must be for legitimate wallet recovery purposes
- **Legal Compliance**: Contributors must ensure their contributions comply with applicable laws
- **Ethical Behavior**: No contributions that facilitate unauthorized access or illegal activities
- **Respectful Communication**: Maintain professional and respectful communication
- **Security Awareness**: Be mindful of security implications in all contributions

## How to Contribute

### Reporting Issues

1. **Search Existing Issues**: Check if the issue has already been reported
2. **Use Issue Templates**: Follow the provided templates for bug reports and feature requests
3. **Provide Details**: Include system information, error messages, and reproduction steps
4. **Security Issues**: Report security vulnerabilities privately to the maintainers

### Submitting Code Changes

1. **Fork the Repository**: Create a personal fork of the project
2. **Create a Branch**: Use descriptive branch names (e.g., `feature/cuda-optimization`, `fix/memory-leak`)
3. **Make Changes**: Follow the coding standards and guidelines below
4. **Test Thoroughly**: Ensure all tests pass and add new tests for new features
5. **Submit Pull Request**: Provide a clear description of changes and their purpose

### Development Setup

1. **Clone Your Fork**:
   ```bash
   git clone https://github.com/your-username/btc_recovery.git
   cd btc_recovery
   ```

2. **Install Dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential cmake libssl-dev pkg-config
   
   # Optional: CUDA Toolkit for GPU support
   # Optional: OpenCL development libraries
   ```

3. **Build the Project**:
   ```bash
   ./scripts/build.sh
   ```

4. **Run Tests**:
   ```bash
   cd build
   ctest --output-on-failure
   ```

## Coding Standards

### C++ Guidelines

- **Standard**: Use C++17 features and standards
- **Formatting**: Follow consistent indentation (4 spaces, no tabs)
- **Naming Conventions**:
  - Classes: `PascalCase` (e.g., `RecoveryEngine`)
  - Functions/Methods: `snake_case` (e.g., `test_password`)
  - Variables: `snake_case` with trailing underscore for members (e.g., `password_length_`)
  - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_PASSWORD_LENGTH`)

- **Documentation**: Use Doxygen-style comments for public APIs
- **Error Handling**: Use exceptions for error conditions, not return codes
- **Memory Management**: Prefer smart pointers over raw pointers
- **Thread Safety**: Document thread safety guarantees for all public APIs

### CUDA Guidelines

- **Kernel Design**: Optimize for different GPU architectures
- **Memory Management**: Use appropriate memory types (global, shared, constant)
- **Error Checking**: Always check CUDA API return values
- **Architecture Support**: Support both discrete and integrated GPUs
- **Power Efficiency**: Consider power constraints for mobile/embedded devices

### OpenCL Guidelines

- **Portability**: Ensure code works across different OpenCL implementations
- **Error Handling**: Check all OpenCL API calls for errors
- **Kernel Optimization**: Optimize for different device types
- **Memory Patterns**: Use efficient memory access patterns

## Testing Requirements

### Unit Tests

- **Coverage**: Aim for >80% code coverage for new features
- **Framework**: Use Google Test for C++ unit tests
- **Naming**: Test files should end with `_test.cpp`
- **Organization**: Mirror the source directory structure in tests

### Integration Tests

- **GPU Testing**: Test with different GPU types when possible
- **Configuration Testing**: Test various configuration combinations
- **Error Conditions**: Test error handling and edge cases

### Performance Tests

- **Benchmarking**: Include performance benchmarks for critical paths
- **Regression Testing**: Ensure changes don't degrade performance
- **Memory Usage**: Monitor memory usage patterns

## Documentation Requirements

### Code Documentation

- **Public APIs**: Document all public classes, methods, and functions
- **Complex Logic**: Explain non-obvious algorithms and optimizations
- **GPU Kernels**: Document kernel design and optimization strategies
- **Configuration**: Document all configuration options

### User Documentation

- **Setup Guides**: Keep setup instructions current and accurate
- **Examples**: Provide working examples for new features
- **Troubleshooting**: Document common issues and solutions
- **Performance Tuning**: Provide guidance for optimization

## Security Considerations

### Code Security

- **Input Validation**: Validate all user inputs
- **Memory Safety**: Avoid buffer overflows and memory leaks
- **Cryptographic Operations**: Use established cryptographic libraries
- **Sensitive Data**: Handle passwords and keys securely

### Operational Security

- **No Hardcoded Secrets**: Never commit API keys, passwords, or certificates
- **Secure Defaults**: Use secure default configurations
- **Audit Trail**: Log security-relevant operations
- **Access Control**: Implement appropriate access controls

## Performance Guidelines

### CPU Optimization

- **Multi-threading**: Utilize all available CPU cores efficiently
- **Cache Efficiency**: Optimize memory access patterns
- **SIMD**: Use vectorized operations where appropriate
- **Profiling**: Profile code to identify bottlenecks

### GPU Optimization

- **Memory Bandwidth**: Optimize memory access patterns
- **Occupancy**: Maximize GPU occupancy
- **Divergence**: Minimize thread divergence
- **Memory Hierarchy**: Use appropriate memory types

### Power Efficiency

- **Laptop Support**: Consider battery life and thermal constraints
- **Integrated Graphics**: Optimize for shared memory architectures
- **Throttling**: Implement thermal throttling mechanisms
- **Power Profiles**: Support different power profiles

## Review Process

### Pull Request Review

1. **Automated Checks**: All CI checks must pass
2. **Code Review**: At least one maintainer must review the code
3. **Testing**: New features must include appropriate tests
4. **Documentation**: Changes must be documented
5. **Performance**: Performance impact must be evaluated

### Review Criteria

- **Functionality**: Does the code work as intended?
- **Security**: Are there any security implications?
- **Performance**: Is the performance acceptable?
- **Maintainability**: Is the code easy to understand and maintain?
- **Compatibility**: Does it work across supported platforms?

## Release Process

### Version Numbering

- **Semantic Versioning**: Use MAJOR.MINOR.PATCH format
- **Breaking Changes**: Increment major version for breaking changes
- **New Features**: Increment minor version for new features
- **Bug Fixes**: Increment patch version for bug fixes

### Release Checklist

- [ ] All tests pass
- [ ] Documentation is updated
- [ ] Performance benchmarks are acceptable
- [ ] Security review is complete
- [ ] Changelog is updated
- [ ] Version numbers are updated

## Getting Help

### Communication Channels

- **GitHub Issues**: For bug reports and feature requests
- **GitHub Discussions**: For general questions and discussions
- **Security Issues**: Contact maintainers privately for security concerns

### Resources

- **Documentation**: Check the `docs/` directory for detailed guides
- **Examples**: Review the `examples/` directory for usage patterns
- **Tests**: Look at existing tests for implementation examples

## Recognition

Contributors will be recognized in:
- **CONTRIBUTORS.md**: List of all contributors
- **Release Notes**: Major contributions highlighted in releases
- **Documentation**: Contributors credited in relevant documentation

Thank you for contributing to the Bitcoin Wallet Recovery System! Your contributions help make legitimate wallet recovery more accessible and efficient.
