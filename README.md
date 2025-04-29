# Makefile Project Template

Steps to configure, build, run, and test the project.

## Building

```bash
make
```

## Testing

Running basic tests:

```bash
make check
```

## Multi-threaded testing

In the root of the project directory, run the following commands to test the multi-threaded nature of this project.

```bash
chmod +x run_tests.sh
./run_tests.sh
```

## Clean

```bash
make clean
```

## Install Dependencies

In order to use git send-mail you need to run the following command:

```bash
make install-deps
```
