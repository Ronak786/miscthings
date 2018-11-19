/************************************************************************
* Project Name :  	ENCODER-DECODER
* File Name :		Decoder.h
* Version :  		v_00_02_01
* Author :  		Charles Guo
* Create Time :  	11/05/2018
* Copyright @ Mos Technologies Pty Ltd. 2017. All rights reserved.
************************************************************************
* History:
* <author> <timer> <version> <desc>
*
************************************************************************/
#ifndef DECODER_H
#define DECODER_H

extern "C" {
/**
* \brief Set the parametres of this NDcode decoder
* \param version_id : The version of NDcode used to encode the message.
					  There are two version available (0 & 1).
* \param layers_number: Number of layers.
						There is only one layer number (5) available in version 0.
						There are three layer numbers (5, 6, 7) available in version 1.
* \param reserved: used to be the decrypted_flag. Now it is fixed internally, do not use anymore.
* \param spiral_to_layered_flag: A bool value to determine whether to convert the bits array into layered pattern or not
								 This parameter is only available in version 1.
* \return 0: Pass; Otherwise: ERROR CODE
*/

int set_decoder_paras(int version_id, int layers_number, bool reserved, bool spiral_to_layered_flag);

/**
 * \brief A decoder to corvert a bits array into a hexadecimal message and perform an offline check to ensure that the NDCode belongs to the customer set
 * \param bits_array : Pointer to the input array of the binary dots value (c)
 * \param bits_array_size : Size of to the input array of the binary dots value (c)
 * \param msg : Pointer to the output message in hexidecimal string format
 * \param msg_len : Length of the output message in hexidecimal string format
 * \param mode_id : the output security mode ID only used in version 1.
 * \return 0: Pass; Otherwise: ERROR CODE
 */
int decoder(int *bits_array, size_t bits_array_size, char *msg, int msg_len, uint8_t *mode_id);

}
#endif
