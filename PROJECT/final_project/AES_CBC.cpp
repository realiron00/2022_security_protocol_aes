#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "aes_protocol.h"

//������ ���̸� �˷��ִ� �Լ�
int FileSize(const char* fname)
{
	FILE* myfile = fopen(fname, "rb");
	int file_length; //file_length�� ������ ���̸� ����

	fseek(myfile, 0, SEEK_END);
	file_length = ftell(myfile);
	fclose(myfile);

	return file_length; //file_length ��ȯ
}

//������ �е��ϴ� �Լ�
void File_Padding(const char* input, const char* output)
{
	FILE* infile = fopen(input, "rb");
	FILE* outfile = fopen(output, "wb");

	int file_len;
	file_len = FileSize(input);

	int num_blocks = file_len / 16 + 1;
	int remainder = file_len - (num_blocks - 1) * 16;

	byte nblock[16]; //�е��� ���ص� �Ǵ� ���(������ ��� ������)

	for (int i = 0; i < num_blocks - 1; i++) {
		fread((char*)nblock, sizeof(byte), 16, infile);
		fwrite((char*)nblock, sizeof(byte), 16, outfile); 
	}//������ ��� �������� output���Ͽ� �Ű���

	byte last_block[16]; //������ ���
	byte ch;

	if (remainder == 0) {
		fclose(infile);
		fclose(outfile);
	}//������ ����� ���ٸ� �״�� ����
	else { //�е��� ����� �ִٸ�
		for (int i = 0; i < remainder; i++) {
			fread((char*)&ch, sizeof(byte), 1, infile);
			last_block[i] = ch;
		} //���� �����ʹ� �Ű���

		byte padding_number = 0x00 + (16 - remainder);
		for (int i = remainder; i < 16; i++) {
			last_block[i] = padding_number; //������� ���� ������ ���� ���ڸ� �־ �е���
		}
		fwrite((char*)last_block, sizeof(byte), 16, outfile); //�е����� �� ������ ����� output���Ͽ� �Ű���

		fclose(infile);
		fclose(outfile);
	}
}

//�е��� ����� �Լ�
void erase_padding(const char* pad_name, const char* origin_name)
{
	FILE* infile = fopen(pad_name, "rb");
	FILE* outfile = fopen(origin_name, "wb");

	int file_len;
	file_len = FileSize(pad_name);
	int num_blocks = file_len / 16 + 1;

	byte buffer[16];
	int x;

	for (int i = 0; i < num_blocks - 2; i++) {
		fread((char*)buffer, sizeof(byte), 16, infile);
		fwrite((char*)buffer, sizeof(byte), 16, outfile); 
	}//�е��� �κ��� �� ��ϱ��� origin���Ͽ� ����

	byte last_block[16];
	fread((char*)last_block, sizeof(byte), 16, infile); //������ ����� last_block�� ����
	for (int i = 0; i < 16; i++) {
		if (last_block[16 - i] == i) {
			for (int j = 16 - i; j < 16; j++) {
				if (last_block[j] == i) x = i; //������ ��Ͽ��� ���� i�� ���������� i�� ��ŭ �����Ѵٸ� x=i
				else x = 0;
			}
			if (x == i) break;
		}
		else x = 0;
	}
	for (int i = 0; i < 16 - x; i++) { //0���� 16-x������ �κ��� origin���Ͽ� ����
		fwrite((char*)&last_block[i], sizeof(byte), 1, outfile);
	}

	fclose(infile);
	fclose(outfile);
}

//CBC��� AES ���� ��ȣȭ
void file_AES_CBC(const char *pt_name, byte IV[16], byte Key[16], const char *ct_name)
{
	byte rk[11][16];
	FILE *pt_file;
	FILE *ct_file;

	byte pt_block[16];
	byte ct_block[16];

	KeySchedule(Key, rk); //����Ű ȹ��

	const char* pad_file = "AES_CBC-Padded.bin";
	File_Padding(pt_name, pad_file); //������ �е��Ͽ� ����

	int file_len;
	file_len = FileSize(pad_file);
	int num_blocks = file_len / 16 + 1;

	pt_file = fopen(pad_file, "rb");
	ct_file = fopen(ct_name, "wb");

	for (int i = 0; i < num_blocks - 1; i++) {
		if (i == 0) { //CBC ù���
			for (int j = 0; j < 16; j++) {
				fread((char*)&pt_block[j], sizeof(byte), 1, pt_file); //CBC�� ù����� pt_block�� ����
				pt_block[j] = pt_block[j] ^ IV[j]; //pt_block�� �ʱ⺤�͸� XOR���� ����
			}
			AES_Encrypt(pt_block, rk, ct_block); //pt_block�� ��ȣȭ�Ͽ� ct_block�� 1��� �Է�
			fwrite((char*)ct_block, sizeof(byte), 16, ct_file); //��ȣȭ���Ͽ� ct_block�� 1��� �Է�
		}
		else { //ù��� ����
			for (int j = 0; j < 16; j++) {
				fread((char*)&pt_block[j], sizeof(byte), 1, pt_file); //pt_block�� �������� 1��Ͼ� �Է�
				pt_block[j] = pt_block[j] ^ ct_block[j]; //pt_block�� ������ ��ȣ�� ����� XOR���� ����
			}
			AES_Encrypt(pt_block, rk, ct_block); //pt_block�� ��ȣȭ�Ͽ� ct_block�� 1��� �Է�
			fwrite((char*)ct_block, sizeof(byte), 16, ct_file); //��ȣȭ���Ͽ� ct_block�� 1��Ͼ� �Է�
		}
	}

	fclose(pt_file);
	remove(pad_file); //�е��Ǿ��� ������ ����
	fclose(ct_file);
}

//CBC��� AES ���� ��ȣȭ
void Inv_file_AES_CBC(const char *ct_name, byte IV[16], byte Key[16], const char *pt_name)
{
	byte rk[11][16];
	FILE *pt_file;
	FILE *ct_file;

	byte pt_block[16];
	byte ct1_block[16];
	byte ct2_block[16];

	KeySchedule(Key, rk); //����Ű ȹ��

	const char* pad_file = "Inv_AES_CBC-Padded.bin"; //�е��� ����� ���� ����

	int file_len;
	file_len = FileSize(ct_name);
	int num_blocks = file_len / 16 + 1;

	ct_file = fopen(ct_name, "rb");
	pt_file = fopen(pad_file, "wb");

	for (int i = 0; i < num_blocks - 1; i++) {
		if (i == 0) { //CBC ù���
			fread((char*)ct1_block, sizeof(byte), 16, ct_file); //��ȣ�������� ù����� ct1_block�� ����
			AES_Decrypt(ct1_block, rk, pt_block); //ct1_block�� ��ȣȭ�Ͽ� pt_block�� �� 1��� �Է�
			for (int j = 0; j < 16; j++) {
				pt_block[j] = pt_block[j] ^ IV[j]; //pt_block�� �ʱ⺤�͸� XOR���� ����
				fwrite((char*)&pt_block[j], sizeof(byte), 1, pt_file); //pad_file�� pt_block 1��� ����
			}
		}
		else { //ù��� ����
			fread((char*)ct2_block, sizeof(byte), 16, ct_file); //ct2_block�� ��ȣ�������� 1��Ͼ� �Է�
			AES_Decrypt(ct2_block, rk, pt_block); //ct2_block�� ��ȣȭ�Ͽ� pt_block�� 1��� �Է�
			for (int j = 0; j < 16; j++) {
				pt_block[j] = pt_block[j] ^ ct1_block[j]; //pt_block�� ������ ��ȣ�� 1����� XOR���� ����
				ct1_block[j] = ct2_block[j]; //ct1_block�� ��ȣ�� 1����� ����
				fwrite((char*)&pt_block[j], sizeof(byte), 1, pt_file); //pad_file�� �е��� ���Ե� ���� �Է�
			}
		}
	}

	fseek(pt_file, 0, SEEK_SET);
	erase_padding(pad_file, pt_name); //pad_file�� �е��κ��� �����Ͽ� pt���Ͽ� ����

	fclose(pt_file);
	remove(pad_file);
	fclose(ct_file); // ���� �ݱ�
}