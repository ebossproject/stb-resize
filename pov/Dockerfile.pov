FROM python:3.11-slim

# Set working directory in the container
WORKDIR /pov

RUN pip install pillow pyyaml

ARG PORT=5000
ARG SERVER=server

ENV PORT=${PORT}
ENV SERVER=${SERVER}

# Copy the script and test files into the container
COPY pov/pov.py .
COPY pov/*.png .
COPY pov/*.yaml .

# Set the command to run the script
CMD ["python3", "pov.py"]
