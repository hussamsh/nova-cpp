#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <bitset>
#include <string>
#include <iostream>
#include <math.h>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>

using namespace cv;
using namespace std;


bool logging = false;


string imagePath;
string outputPath;
// 1 -> logistic , 2 -> DH , 3 -> Henon
int algorithm;
// 1 -> encrypt , 2 -> decrypt
int operation;

double parameters[4];



bitset<24> BgrToBinary(int blue, int green, int red)
{
	return bitset<24>(bitset<8>(blue).to_string() + bitset<8>(green).to_string() + bitset<8>(red).to_string());
}

array<int, 3> extractBGR(bitset<24> pixel)
{
	string temp;
	int i = 23;

	while (i > 15)
		temp += pixel[i--] ? "1" : "0";

	int blue = (bitset<8>(temp).to_ulong());
	temp.clear();

	while (i > 7)
		temp += pixel[i--] ? "1" : "0";
	
	int green = (bitset<8>(temp).to_ulong());
	temp.clear();

	while (i >= 0)
		temp += pixel[i--] ? "1" : "0";

	int red = (bitset<8>(temp).to_ulong());
	temp.clear();

	return { blue , green , red};
}

double nextIteration() {
	
	switch (algorithm)
	{
	case 1:
			return parameters[0] * parameters[1] * (1 - parameters[1]);
	case 2:
		{
			double power = pow(parameters[1] - parameters[2], 2.0);
			return parameters[0] * power * (pow(parameters[2], 2.0) - power);
		}
	case 3:
		{
			double result = 1 - (parameters[2] * pow(parameters[1], 2.0)) + parameters[0];
			parameters[0] = parameters[3] * parameters[1];
			return result;
		}
		
	}

}

void encrypt(string imagePath, double growthRate, double initialCondition, double gParam , string outputPath)
{

	cout << "Encrypting : " << imagePath << "\n" << endl;

	// Variables
	bitset<24> LSB;
	bitset<24> prevPixel(0);
	bitset<24> rgbConversion;
	bitset<24> encryptedPixel;
	bitset<24> xorVal;
	
	array<int, 3> BGR;

	uint64_t u;

	int blue;
	int green;
	int red;

	//Algorithm
	Mat img = imread(imagePath);


	for (int i = 0; i < img.rows; i++)
	{
		for (int j = 0; j < img.cols; j++)
		{

			Vec3b intensity = img.at<Vec3b>(i, j);

			blue = intensity.val[0];
			green = intensity.val[1];
			red = intensity.val[2];

			rgbConversion = BgrToBinary(blue, green, red);

			parameters[1] = nextIteration();

			if (isinf(parameters[1])) {
				cout << "Value diverged to infinity, exiting \n";
				cout << "Try again with different initial conditions \n";
				return;
			}

			memcpy(&u, &parameters[1], sizeof u);

			for (int i = 23; i >= 0; i--)
				LSB[i] = ('0' + ((u >> i) & 1)) == '0' ? 0 : 1;

			xorVal = LSB ^ rgbConversion;

			encryptedPixel = xorVal ^ prevPixel;

			BGR = extractBGR(encryptedPixel);

			//Blue 
			intensity.val[0] = BGR[0];
			//Green
			intensity.val[1] = BGR[1];
			//Red
			intensity.val[2] = BGR[2];

			img.at<Vec3b>(i, j) = intensity;

			if (::logging)
			{
				cout << "Cur : " << rgbConversion << endl;
				cout << "LSB : " << LSB << endl;
				cout << "Xor : " << xorVal << endl;
				cout << "Prv : " << prevPixel << endl;
				cout << "Enc : " << encryptedPixel << endl;
				cout << "Blue : " << +BGR[0] << ", Green : " << +BGR[1] << ", Red : " << BGR[2] << endl;
			}

			prevPixel = encryptedPixel;
		}
	}

	cout << "Encryption finished successfully \n";
	imshow("Encrypted", img);
	moveWindow("Encrypted", 40, 30);

	imwrite(outputPath, img);


	return;
}

void decrypt(string imagePath, double growthRate, double initialCondition, double gParam, string outputPath)
{
	cout << "Decrypting : " << imagePath << "\n" << endl ;

	// Variables
	bitset<24> LSB;
	bitset<24> prevPixel(0);
	bitset<24> encryptedPixel;
	bitset<24> originalPixel;
	bitset<24> xorVal;

	array<int, 3> BGR;

	int blue;
	int green;
	int red;

	uint64_t u;

	//Algorithm
	Mat img = imread(imagePath);

	for (int i = 0; i < img.rows; i++)
	{
		for (int j = 0; j < img.cols; j++)
		{

			Vec3b intensity = img.at<Vec3b>(i, j);

			blue = intensity.val[0];
			green = intensity.val[1];
			red = intensity.val[2];

			encryptedPixel = BgrToBinary(blue, green, red);

			parameters[1] = nextIteration();

			if (isinf(parameters[1])) {
				cout << "Value diverged to infinity, exiting \n";
				cout << "Try again with different initial conditions \n";
				return;
			}

			memcpy(&u, &parameters[1], sizeof u);

			for (int i = 23; i >= 0; i--)
				LSB[i] = ('0' + ((u >> i) & 1)) == '0' ? 0 : 1;

			xorVal = LSB ^ encryptedPixel;

			originalPixel = xorVal ^ prevPixel;

			BGR = extractBGR(originalPixel);

			//Blue 
			intensity.val[0] = BGR[0];
			//Green
			intensity.val[1] = BGR[1];
			//Red
			intensity.val[2] = BGR[2];

			img.at<Vec3b>(i, j) = intensity;

			if (::logging)
			{
				cout << "Enc : " << encryptedPixel << endl;
				cout << "LSB : " << LSB << endl;
				cout << "Xor : " << xorVal << endl;
				cout << "Prv : " << prevPixel << endl;
				cout << "Org : " << originalPixel << endl;
				cout << "Blue : " << +BGR[0] << ", Green : " << +BGR[1] << ", Red : " << BGR[2] << endl;
			}

			prevPixel = encryptedPixel;

		}
	}

	imshow("Original", img);
	moveWindow("Original", 200, 30);

	imwrite(outputPath, img);
}

void mapParamsInput() 
{
	cout << "Available Algorithms " << endl;

	cout << "1 => Logistic map encryption" << endl;
	cout << "2 => Double humped map encryption" << endl;
	cout << "3 => Henon map encryption" << endl << endl;

	do {
		cin.clear();
		cout << "Enter number of desired algorithm : ";

		cin >> algorithm;

		if (cin.fail() || !(algorithm == 1 || algorithm == 2 || algorithm == 3)) {
			cout << "Invalid input, please enter a number between 1 - 3" << endl << "\n";
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}

	} while (!(algorithm == 1 || algorithm == 2 || algorithm == 3));

	cout << endl << "------------------------------------------------- \n";
	

	switch (algorithm)
	{
	case 1:
	{
		cout << "   Logistic map encryption \n \n";
		cout << "   x_{n+1} = r (1- x_n) \n";
		cout << "------------------------------------------------- \n \n";

		cout << "Make sure your input will make the map chaotic" << endl << "\n";

		string inputs[] = { "r (Growth rate) : " , "x_n (Initial condition) : "};
		bool failed = true;

		for (size_t i = 0; i < 2; i++)
		{
			do {
				cin.clear();
				cout << inputs[i];

				cin >> parameters[i];

				if (cin.fail()) {
					cout << "Invalid input, please enter a number" << endl << "\n";
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					failed = true;
				}
				else {
					failed = false;
				}

			} while (failed);

			cout << "\n";
		}
		break;
	}
	case 2:
	{
		cout << "   Double humped map encryption \n \n";
		cout << "   x_{n+1} = r (x_n - c)^2 (c^2 - (x_n - 1)^2) \n";
		cout << "------------------------------------------------- \n \n";

		cout << "Make sure your input will make the map chaotic" << endl << "\n";

		string inputs[] = { "r (Growth rate) : " , "x_n (Initial condition) : " , "c (Generalization parameter) : " };
		bool failed = true;

		for (size_t i = 0; i < 3; i++)
		{
			do {
				cin.clear();
				cout << inputs[i];

				cin >> parameters[i];

				if (cin.fail()) {
					cout << "Invalid input, please enter a number" << endl << "\n";
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					failed = true;
				}
				else {
					failed = false;
				}

			} while (failed);

			cout << "\n";
		}

		break;
	}
	case 3:
	{
		cout << "   Henon map encryption \n \n";
		cout << "   x_{n+1} = 1 - (a - (x_n)^2) + y_n \n";
		cout << "   y_{n+1} = b * x_n \n";
		cout << "------------------------------------------------- \n \n";

		cout << "Make sure your input will make the map chaotic" << endl << "\n";

		string inputs[] = { "y : " , "x : " , "a : " , "b : "};
		bool failed = true;

		for (size_t i = 0; i < 4; i++)
		{
			do {
				cin.clear();
				cout << inputs[i];

				cin >> parameters[i];

				if (cin.fail()) {
					cout << "Invalid input, please enter a number" << endl << "\n";
					cin.clear();
					cin.ignore(numeric_limits<streamsize>::max(), '\n');
					failed = true;
				}
				else {
					failed = false;
				}

			} while (failed);

			cout << "\n";
		}
		break;
	}
	}


}

void showWelcomeScreen() {
	cout << R"( _____  ___      ______  ___      ___  __      
(\"   \|"  \    /    " \|"  \    /"  |/""\     
|.\\   \    |  // ____  \\   \  //  //    \    
|: \.   \\  | /  /    ) :)\\  \/. .//' /\  \   
|.  \    \. |(: (____/ //  \.    ////  __'  \  
|    \    \ | \        /    \\   //   /  \\  \ 
 \___|\____\)  \"_____/      \__/(___/    \___)          
	)" << endl;


	mapParamsInput();;

	cout << "Opeartions : (1) Encrypt or (2) Decrypt "<< endl;

	do {
		cin.clear();
		cout << "Enter operation number : ";

		cin >> operation;

		if (cin.fail() || !(operation == 1 || operation== 2)) {
			cout << "Invalid input, please enter either 1 or 2" << endl << "\n";
			cin.clear();
			cin.ignore(numeric_limits<streamsize>::max(), '\n');
		}

	} while (!(operation == 1 || operation == 2));

	bool isImage = false;

	ifstream ifile;
	string operationStr = (operation == 1 ? "encrypt" : "decrypt");

	while (!ifile || !isImage)
	{
		cout << "path of image to " << operationStr << " : ";
		getline(cin >> ws, imagePath);


		ifile.open(imagePath);

		if (!ifile) {
			cout << "file doesn't exist, try again" << endl << "\n";
		}
		else {
			string extn = imagePath.substr(imagePath.find_last_of(".") + 1);

			if (extn == "png" || extn == "jpg") {
				isImage = true;
				cout << operationStr << "ed image will be written to the same folder" << endl << "\n";
			}
			else {
				cout << "file chosen is not an image, try again" << endl<< "\n";
				isImage = false;
			}
		}

		ifile.close();
	}


	size_t found;
	found = imagePath.find_last_of(".");
	outputPath = imagePath;
	outputPath.replace(found, sizeof(outputPath), "_" + operationStr + "ed.png");
;}


int main() {


	showWelcomeScreen();

	if (operation == 1)
	{
		encrypt(imagePath, parameters[0], parameters[1], parameters[2], outputPath);
	}else if(operation == 2)
	{
		decrypt(imagePath, parameters[0], parameters[1], parameters[2], outputPath);
	}

	waitKey(0);

	return 0;
}