#include "SGXNN_t.h"
#include "SGXNN.h"

#include "sgx_trts.h"
#include <stdarg.h>
#include <stdio.h>      /* vsnprintf */
#include <stdlib.h>

#include "sgx_tcrypto.h"
#include <string.h>
#include "sgx_tseal.h"
#include "sealing/sealing.h"


#include "math.h"
#include "../../App/App/App.h"


#define BUFLEN 2048
#define SGX_AESGCM_MAC_SIZE 16
#define SGX_AESGCM_IV_SIZE 12
static sgx_aes_gcm_128bit_key_t key = { 0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf };

// NN described in array

// Number of training samples
const int nTraining = 60000;

// Image size in MNIST database
const int width = 28;
const int height = 28;

// n1 = Number of input neurons
// n2 = Number of hidden neurons
// n3 = Number of output neurons
// epochs = Number of iterations for back-propagation algorithm
// learning_rate = Learing rate
// momentum = Momentum (heuristics to optimize back-propagation algorithm)
// epsilon = Epsilon, no more iterations if the learning error is smaller than epsilon

const int n1 = width * height; // = 784, without bias neuron
const int n2 = 128;
const int n3 = 10; // Ten classes: 0 - 9
const int epochs = 512;
const double learning_rate = 1e-3;
const double momentum = 0.9;
const double epsilon = 1e-3;

NeuralNetwork *nn;
double test[3];

// From layer 1 to layer 2. Or: Input layer - Hidden layer
double delta1[785][129], out1[785];

// From layer 2 to layer 3. Or; Hidden layer - Output layer
double delta2[129][11], in2[129], out2[129], theta2[129];

// Layer 3 - Output layer
double in3[11], out3[11], theta3[11];
double expected[11];


// Image. In MNIST: 28x28 gray scale images.
int d[width + 1][height + 1];

void init_array()
{
	uint32_t val;
	// sgx_read_rand((unsigned char *) &val, 4);
	nn = (NeuralNetwork*)malloc(sizeof(NeuralNetwork));
	test[0] = 0.1;
	test[1] = 1.1;
	test[2] = 2.1;

	// Initialization for weights from Input layer to Hidden layer
	for (int i = 1; i <= n1; ++i) {
		for (int j = 1; j <= n2; ++j) {
			sgx_read_rand((unsigned char *)&val, 4);
			int sign = val % 2;

			sgx_read_rand((unsigned char *)&val, 4);
			nn->w1[i][j] = (double)(val % 6) / 10.0;
			if (sign == 1) {
				nn->w1[i][j] = -nn->w1[i][j];
			}
		}
	}

	// Initialization for weights from Hidden layer to Output layer
	for (int i = 1; i <= n2; ++i) {
		for (int j = 1; j <= n3; ++j) {
			sgx_read_rand((unsigned char *)&val, 4);
			int sign = val % 2;

			sgx_read_rand((unsigned char *)&val, 4);
			nn->w2[i][j] = (double)(val % 10 + 1) / (10.0 * n3);
			if (sign == 1) {
				nn->w2[i][j] = -nn->w2[i][j];
			}
		}
	}
}

// sigmoid function
double sigmoid(double x)
{
	return 1.0 / (1.0 + exp(-x));
}

// Forward process - perceptron
void perceptron()
{
	for (int i = 1; i <= n2; ++i) {
		in2[i] = 0.0;
	}

	for (int i = 1; i <= n3; ++i) {
		in3[i] = 0.0;
	}

	for (int i = 1; i <= n1; ++i) {
		for (int j = 1; j <= n2; ++j) {
			in2[j] += out1[i] * nn->w1[i][j];
		}
	}

	for (int i = 1; i <= n2; ++i) {
		out2[i] = sigmoid(in2[i]);
	}

	for (int i = 1; i <= n2; ++i) {
		for (int j = 1; j <= n3; ++j) {
			in3[j] += out2[i] * nn->w2[i][j];
		}
	}

	for (int i = 1; i <= n3; ++i) {
		out3[i] = sigmoid(in3[i]);
	}
}

// Norm L2 error
double square_error()
{
	double res = 0.0;
	for (int i = 1; i <= n3; ++i) {
		res += (out3[i] - expected[i]) * (out3[i] - expected[i]);
	}
	res *= 0.5;
	return res;
}

// Back Propagation Algorithm
void back_propagation()
{
	double sum;

	for (int i = 1; i <= n3; ++i) {
		theta3[i] = out3[i] * (1 - out3[i]) * (expected[i] - out3[i]);
	}

	for (int i = 1; i <= n2; ++i) {
		sum = 0.0;
		for (int j = 1; j <= n3; ++j) {
			sum += nn->w2[i][j] * theta3[j];
		}
		theta2[i] = out2[i] * (1 - out2[i]) * sum;
	}

	for (int i = 1; i <= n2; ++i) {
		for (int j = 1; j <= n3; ++j) {
			delta2[i][j] = (learning_rate * theta3[j] * out2[i]) + (momentum * delta2[i][j]);
			nn->w2[i][j] += delta2[i][j];
		}
	}

	for (int i = 1; i <= n1; ++i) {
		for (int j = 1; j <= n2; j++) {
			delta1[i][j] = (learning_rate * theta2[j] * out1[i]) + (momentum * delta1[i][j]);
			nn->w1[i][j] += delta1[i][j];
		}
	}
}

// Learning process: perceptron - back propagation
int learning_process()
{
	for (int i = 1; i <= n1; ++i) {
		for (int j = 1; j <= n2; ++j) {
			delta1[i][j] = 0.0;
		}
	}

	for (int i = 1; i <= n2; ++i) {
		for (int j = 1; j <= n3; ++j) {
			delta2[i][j] = 0.0;
		}
	}

	for (int i = 1; i <= epochs; ++i) {
		perceptron();
		back_propagation();
		if (square_error() < epsilon) {
			return i;
		}
	}
	return epochs;
}

// reading input - gray scale image and the corresponding label
int input(char *data)
{
	// reading image
	char number;
	//ocall_read_one(encMsg);

	//printf("Data read from app: %s\n", data);
	printf("Decrypting...\n");

	size_t decMessageLen = 785;
	char *decMessage = (char *)malloc((decMessageLen + 1) * sizeof(char));
	size_t encMessageLen = (SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE + 785);
	decryptMessage(data, encMessageLen, decMessage, decMessageLen);
	decMessage[decMessageLen] = '\0';
	//printf("Decrypted msg: %s\n", decMessage);


	// counter
	int cnt = 0;

	for (int j = 1; j <= height; ++j) {
		for (int i = 1; i <= width; ++i) {
			// ocall_read_image(&number);
			if (decMessage[cnt] == 0) {
				d[i][j] = 0;
			}
			else {
				d[i][j] = 1;
			}
			cnt++;
		}
	}

	/*
	printf("Image:\n");
	for (int j = 1; j <= height; ++j) {
		for (int i = 1; i <= width; ++i) {
			printf("%d", d[i][j]);
		}
		printf("\n");
	}
	*/

	for (int j = 1; j <= height; ++j) {
		for (int i = 1; i <= width; ++i) {
			int pos = i + (j - 1) * width;
			out1[pos] = d[i][j];
		}
	}

	// reading label
	// ocall_read_label(&number);
	for (int i = 1; i <= n3; ++i) {
		expected[i] = 0.0;
	}
	// expected[number + 1] = 1.0;
	expected[decMessage[784] + 1] = 1.0;

	// printf("Label: %d\n", (int)number);
	number = decMessage[784];
	free(decMessage);
	return (int)(number);
}

void save_nn() {
	sgx_status_t ocall_status, sealing_status;
	sealing_status = SGX_SUCCESS;
	int ocall_ret;
	//size_t sealed_sizea = sizeof(sgx_sealed_data_t) + sizeof(NeuralNetwork);
	size_t sealed_size = sgx_calc_sealed_data_size(0, sizeof(NeuralNetwork));
	//printf("size of nn is %d\n", sealed_sizea);
	printf("size of NN is %d\n", sizeof(NeuralNetwork));
	uint8_t* sealed_data = (uint8_t*)malloc(sealed_size);

	//sealing_status = seal_nn(nn, (sgx_sealed_data_t*)sealed_data, sealed_size);
	/*sealing_status =  sgx_seal_data(0, NULL, 
		sizeof(NeuralNetwork), (uint8_t*)nn,
		(uint32_t)sealed_size, (sgx_sealed_data_t*)sealed_data);
	free(nn);
	if (sealing_status != SGX_SUCCESS) {
		free(sealed_data);
		printf("Sealing NN failed!\n");
		print_error_message(sealing_status);
		return;
	}*/

	ocall_status = ocall_save_nn(&ocall_ret, (uint8_t*)nn, sizeof(NeuralNetwork));
	free(sealed_data);
	if (ocall_ret != 0 || ocall_status != SGX_SUCCESS) {
		printf("Saving NN failed!");
		return;
	}
	else
		printf("%s", "saved NN mmodel succcessfully");
}

void load_model() {
	sgx_status_t ocall_status, sealing_status;
	int ocall_ret;

	size_t sealed_size = sizeof(sgx_sealed_data_t) + sizeof(NeuralNetwork);
	uint8_t* sealed_data = (uint8_t*)malloc(sealed_size);
	size_t plaintext_size = sizeof(NeuralNetwork);
	printf("%d\n", plaintext_size);

	NeuralNetwork *network;
	network = (NeuralNetwork*)malloc(plaintext_size);
	printf("size of NN is %d\n", sizeof(NeuralNetwork));

	ocall_status = ocall_load_nn(&ocall_ret, (uint8_t*)network, plaintext_size);
	if (ocall_ret != 0 || ocall_status != SGX_SUCCESS) {
		free(sealed_data);
		printf("Loading NN failed!\n");
	}
	else
	{
		printf("%s", "loaded NN");
	}

	/*uint32_t plaintext_size = sizeof(NeuralNetwork);
	nn = (NeuralNetwork*)malloc(plaintext_size);
	sealing_status = unseal_nn((sgx_sealed_data_t*)sealed_data, nn, plaintext_size);
	free(sealed_data);
	if (sealing_status != SGX_SUCCESS) {
		free(nn);
		printf("Unsealing NN failed!\n");
	}*/

}

int predict() {
	perceptron();
	int result = 1;
	for (int i = 2; i <= n3; ++i) {
		if (out3[i] > out3[result]) {
			result = i;
		}
	}
	--result;

	return result;
}

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
void printf(const char *fmt, ...)
{
	char buf[BUFSIZ] = { '\0' };
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, BUFSIZ, fmt, ap);
	va_end(ap);
	ocall_print_string(buf);
}

void printf_helloworld()
{
	printf("Hello World\n");
}

void decryptMessage(char *encMessageIn, size_t len, char *decMessageOut, size_t lenOut)
{
	uint8_t *encMessage = (uint8_t *)encMessageIn;
	uint8_t p_dst[BUFLEN] = { 0 };
	sgx_status_t decStatus = sgx_rijndael128GCM_decrypt(
		&key,
		encMessage + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
		lenOut,
		p_dst,
		encMessage + SGX_AESGCM_MAC_SIZE, SGX_AESGCM_IV_SIZE,
		NULL, 0,
		(sgx_aes_gcm_128bit_tag_t *)encMessage);
	if (decStatus != SGX_SUCCESS)
		printf("Decrypting failed");
	memcpy(decMessageOut, p_dst, lenOut);
}

void encryptMessage(char *decMessageIn, size_t len, char *encMessageOut, size_t lenOut)
{
	uint8_t *origMessage = (uint8_t *)decMessageIn;
	uint8_t p_dst[BUFLEN] = { 0 };

	// Generate the IV (nonce)
	sgx_read_rand(p_dst + SGX_AESGCM_MAC_SIZE, SGX_AESGCM_IV_SIZE);

	sgx_rijndael128GCM_encrypt(
		&key,
		origMessage, len,
		p_dst + SGX_AESGCM_MAC_SIZE + SGX_AESGCM_IV_SIZE,
		p_dst + SGX_AESGCM_MAC_SIZE, SGX_AESGCM_IV_SIZE,
		NULL, 0,
		(sgx_aes_gcm_128bit_tag_t *)(p_dst));
	memcpy(encMessageOut, p_dst, lenOut);
}
