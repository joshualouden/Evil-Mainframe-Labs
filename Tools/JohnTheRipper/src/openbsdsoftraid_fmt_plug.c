/*
 * Copyright (c) 2014 Thiébaud Weksteen <thiebaud at weksteen dot fr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Fixed BE issues, and build problems (Fall 2014), JimF.
 */

#include "arch.h"

#if FMT_EXTERNS_H
extern struct fmt_main fmt_openbsd_softraid;
#elif FMT_REGISTERS_H
john_register_one(&fmt_openbsd_softraid);
#else

#include "aes.h"
#include "hmac_sha.h"
#include "sha.h"
#include "common.h"
#include "formats.h"
#include "bcrypt_pbkdf.h"
#include "pbkdf2_hmac_sha1.h"
#include "loader.h"
#ifdef _OPENMP
static int omp_t = 1;
#include <omp.h>
#ifndef OMP_SCALE
#define OMP_SCALE                   1
#endif
#endif
#include "memdbg.h"

#define FORMAT_LABEL                "OpenBSD-SoftRAID"
#define FORMAT_NAME                 ""
#define FORMAT_TAG                  "$openbsd-softraid$"
#define FORMAT_TAG_LEN              (sizeof(FORMAT_TAG)-1)
#ifdef SIMD_COEF_32
#define ALGORITHM_NAME              "PBKDF2-SHA1 " SHA1_ALGORITHM_NAME
#else
#define ALGORITHM_NAME              "PBKDF2-SHA1 32/" ARCH_BITS_STR
#endif
#define BENCHMARK_COMMENT           " (8192 iterations)"
#define BENCHMARK_LENGTH            -1
#define PLAINTEXT_LENGTH            125
#define SALT_SIZE                   sizeof(struct custom_salt)
#define SALT_ALIGN                  4
#ifdef SIMD_COEF_32
#define MIN_KEYS_PER_CRYPT          SSE_GROUP_SZ_SHA1
#define MAX_KEYS_PER_CRYPT          SSE_GROUP_SZ_SHA1
#else
#define MIN_KEYS_PER_CRYPT          1
#define MAX_KEYS_PER_CRYPT          1
#endif

#define OPENBSD_SOFTRAID_SALTLENGTH 128
#define OPENBSD_SOFTRAID_KEYS       32
#define OPENBSD_SOFTRAID_KEYLENGTH  64  /* AES-XTS-256 keys are 512 bits long */
#define OPENBSD_SOFTRAID_MACLENGTH  20
#define BINARY_SIZE                 OPENBSD_SOFTRAID_MACLENGTH
#define BINARY_ALIGN                sizeof(uint32_t)

static struct fmt_tests tests_openbsdsoftraid[] = {
	// too long of line was causing my Sparc box to fail to compile this code
	{"\
$openbsd-softraid$8192$c2891132ca5305d1189a7da94d32de29182abc2f56dc641d685e471935f2646e06b79f1d6c102c2f62f3757a20efb0a110b8ae207f9129f0dc5eea8ab05cc8280e0ba2460faf979dbac9f577c4a083349064364556b7ad15468c17c4d794c3da0ddf5990cc66751a6ded8d534531dd9aa9fce2f43e68d6a7200e135beb55e752$311c42d1d8daf1e47e0150c8d4a455a0567b062970c1838faaedcd3e43795545de64971c7598902a6e2c3fffcf8abe2ef78979164d0c9089fbb931c4c9dac8b86c85eeace11095e38487e41eb7b6094d96c339e86686121fbe1c32dbff3c00706926b22ec3a1329f346c599d132105b5d182a380161504d535f9836bb7286331adce1e47e4e251a0249612a94312bb309a6f4558568467731c1ae8c9b910d27102dca2a72228ffde7bfc60004c8ab33ca2b01aa476c4f42f99a3d1f904e3bbc56270edb314a62e92cf68185ace93731ef4ce08dff3c695c45e35b57ed8ab1552114635eb2ff531437ba5c3a08ebf3e73b6bbb7fe1ad98373da349f09284ae819b6a2f6fc5a10aec347f3c2331abc1d6617e77d68f314fdb683294f3ef351869491c4fb096969924215d711c15e5fce533dc5acaed4a473b14c595bababc178e62ef065770716520ecddc7cbf1cbed1250b7e004ab975bc29780c952087ec382bf6e77447720a10a8c2993262a2b21f8a3f47e35daa5b620573626b474d3e8abf8e73164664b041a18fe35c2a1905fad617bf6e6c380fdeeb680fa89b6c6dc7676ad93fde25076ecb8855d623b45af9a16a62a957d85c4c70896019be1827ad9320a69f18bdfc2674f04babdbfcd679c0ef22f7ab2a18818b9b425e61d8c06196a23babd0aefd5a00f1b297a66d973daae40f4dbd9be60d8953fafbd51f7745e2d04b5c80b63ad1f550cd939490b346d4fe7c1fc266d593bcafac0d8989994e174de6d1ef4ce78b3224ea4e68ccbf998654a067558537be332f5cae4b44c18664428d45b71cde5b53bedddf8a7daf47fce212578b72\
7e420c91de0baa1108683dd5b5534e81f4fe945d27fd9d28934afc8d15d95932952c0be717d4d87bb8255bf658a083c3aed643f7a6cfb56fbcbdab9e0a7348b0a3a91e3d560d1ec96f5769551e64beb54a499f6d6dd37e4361d484fe4f7bac4dc26c8a1a2609592d527b134c8212d71b3578217e0ec1da317c69e7e8c39d2d5b2d4073fa9c618a01a092b61613f6f1e41e6ab43d8ca010f177947aeab2884e9a4dd28453ff5bdadb765680733e7af1463ec1b20b879ae01c9256da0207811f956b3950f6db743a9e34a6d8f0fdfa5c47b4f807f0017c2092d72dc19d111711e796ffc4035da3a4caa6a5301491d0473b0d47cd01b705ff11a10263867013a11c65462c311fa5ac9a2598142779b55f09dbec89ac18049c29e5baf3aa38696a3b92d08b02cb10af5389e06058b3ad8be09b121e4e320520413775b7c6fbb3f2b332e3ac0295a4a4dfb4a56ea1c32bc28c149ffaa3b426f5a17a11afe56426b38966c86734654fe05a611c8f025ee4092656c097bbf59743c31508fa9e80ff86a2ae33d401ec316e65eef251d173e9565ffc1672b8b341174427a851a6a4c42554848c637283d13d4ba5b5414b4e61ade6ec7ef7b77186a81adff381e6a79d3dac2c68bf386f100fef1c354221a2ba3d8a7a10460f637eaa152ab79027ab94e5965660de3ed66dac4a0f8e75b85d768e51c8e82a26cb81249ca8d249d8c5cdc8bd55289679d3915a397d31863334df18e2fe3ef9069b064c4ef6b418e5388817040ae9922e5e9f57a8bf3b3fe04748b9cf5068ac86f942b4068853602a6c6c794423569b665b359d5f947c2e5ff194d23d953b435b2b3834513fdfda2b66fcea22883690b1cc56c2fcaa5600895ff8d8ae9e3a6a2b6258ff873242d1128b20e7d1e843ade1bd206b541eba02a214a95cd83860865f947cb4adbd465957055060df05e53fa9ea4b29867c92b224be939d3715be0e61b7aa0e24a8f25bccfa3b7901a3f0a8cb25498d7c9899d435b409220723dcde1d38ab6d4e7cfb42d443c9b65a37\
53891f46adb9bc52574699a7b642955702ed662d04cbe21aeec7c15db7e325dcaa74c85c5e3ed54424642d5bd8d3109c2d4c0079b3d2c5f2da12ad5b25407ae48f6fe4fc653b23a7f2d56a93c898dd0bd59ba02295934c9f7ffb433ef611d51b7c203f374cf9e8b69d4952ccc44593447ad41540270b0e30c349401048cbce10a0e1bae373de15c878982b0af837fb5432cd2471516d1e218296ce462a59fd5412921bbd3f75cf65070f7bafe21105ba83f7ffe8ece71534863c0dd731a2f3c29fff97b8ce798890a1b158a8891bb6f2dd751e75c0cb0db7ea152d7cdc91663f46f85d12ce0015351dba5225b2a87b64cc30518b23e31b2bfbb0b2a5042eeaea1234a57549a3e55ddd708e3380df032e93071b10b3e6902152c90ffd99bda0177a197779341307c5d9f335e698259ade70564eab9d2856aa1aa814211e71ba2885ef9cd5f5bdd225af2f6eebf775cc0bbdb3e519edb7c49a9a1984cc0cc012679aca8fd1d002fa64b2df095b4a9e2b496e3f4b544955c817efb29562cf8b3d2eeccbe4d364ce71d2d12b504b11de4747139ef505bdd12f382eb02fa3f5272b710644a9c20660ca5b4fa74be60984240b555c1f34261ee1d72d9eb2cc680f32b4603865503addc3a1fdc49d2b158d3407a282edd72ef51ad021338fdebf413726e1778e3bc3909b670d3f40e824391c5525b162ea01c29205e12f8e62bdd8cd0f21f6f7b44af4521c2dd23a7f3508e5dc6fffa3365e4ca1cac33bb515a5c5495dc059a94396de7d802758b65bb4cecb90bf69ab4126eab85958cb8b64eedf3a0955ab42cdc98ef90620e10cc854b9c02bfaff60742494a0c3bb34ef6d6bb861b275d975bdc4a10ac922dc70c1b03a4c01943a704af36ec8d79cf2f9ce0f602f01bef4a32edeb8fbba863c945552efc814410ac6bb839349ea65879644003bdda35d40eabdc9dcfb2d67d945b7f111ab62591763a0dd2d338594eff004237e5acce69dd9d2cdbb9ce121bd$5337e4ba9d877a1e84559688386fbc844c5fe557", "password1" },
	{"$openbsd-softraid$8192$2fada40a4b317c3f8829abc2d046fa33fefc73dab99cc0f68577598ff5d673892145b48afe8eee76baaee531097bb60c6888e74097d56530738c6f572dc1a4e603c7e89631cfcfda6bb8bdcdf681960037c05c8e729b4ecd0c247b6cd504c27002cd8356deadb38c628104e307176218371afac51f7382b7dad8cfb7f65b509a$36ce839df6aaea0b274bed8e15783f57de169fe0361c924e0ad6466c6db52cf41fd07f3848b58e15c5e1647e1b9c513c83b4e09f46e3975d08ec615c74b2b722a7f6f230a94a127a278baaa15b911ad986e7ecc5026f3f012a12ad1b20c1490249ff83c3a782732017e7897e304f4c662b04d47b46b64abc11a98c953220f4210df74351d667a9a12d18883f4e309d1c694488c0cfa817afe354624333172b74d1f7e504d64643106d40cc35059276058b6d653faad6463458529890e109ec4313e3be4782ffb761d0444cf4096e54b663bb2cfe219aed10db7f2dc00aef69b34262cd0ae9c0a59f47c74665adf0a210e78e3b16d57aba9eba2bfc519648ac60941e0d736d6b30b52d8c6ae254127b6c156c5fa0388b3a90a592f26b8a9b66032ebaf2ca536fb8696b2dd4ae1d45488dd6c52bb94777ef2840fda049e0a7bb8b860f1a3b6ca5595076942ca985eefe4c1f97ddc6d16e48627f9f2e2aa31c965e67adf2c7594fcda50f8dde15bcb7045b0bae24b57da6f88154e268ffa68378ef424a5a2b988628c7630c59cd56157ea4e653614f8e55427e39d37d07b43647f8c1bafc6c2bc1c00cb37c13fa25e5205ef2f03b9613f8e4baafc48ad2d0b3feee9cd4807984e9e0650e9f639a45ce165e0478422d1e2aad9a3b056876be4b5219467149599dcc352fd8542ce28dc4f1350713b774c65e3bec345f1925a68afb71688fd683beb7a099f4ac13305abc4e2f40efb3c4147ca8b1b5b58cea0eeee71d2884e96e8c88214c86d326f56a6ede525bccff75784857fab35db71ab2cc6d06017fbc43a5e760cdf83608c47b28c98acfbff8a299ec222231aac7064768220414e829fc464f06b09bf0ef74f8eb356abeb253aaa3d6ee916bc19eacfff70fdc9b2f041bcacf107c8f4ccca06be2ec3fbc2c5734399f160e1f964feba8fce796f20c9c4899ca3e638d12c9a30d06f6cd19fb613be75146053dcfe008a6ff2324440695e70145b1c063fe594ad37f0197b4785cdd81f2de8776d8d7eafbe0bf3533c56dd3fd77456a087b17ce29a15b1df5b2129ba9ef54ffc19b67eb0c6c34024cc3f0fbbf3f81489d10a7d28348ceca62f28b040c9bc32fb9771fbf8ddb5ef34677c13f693318090d4d66b6f507817485ef438072892b10d20982ce778fe410699b4e4ab44647a2cf3af34d465784fbf22a387809adb60248d663994cf68d5ce12278ae3421c48874808c035a445cafcee5be55e03b1c744c12a29327e3ff2cb5e78f8ccb00a2bef88e122b1381a459351bd45260ffb7b6be334719551750c313adf3a9b777095386e78786837e05c4c56fb5093eef9ba8365a103c78e36755619e60ad6781d4d9d1c2b7ee7b471d4c41e525e08d3a60c98916005c13dbb0afe8c4ccf2160390e7a955f208fcf9d59cdaf28ef917b25f5158755288a6f52ad46bf96e7af2a02a35d582ae8c412810355f6973ee9f99d28112374bbf11dd7fd27f7ff48703b85b1167061555a89a66bfbe89f981ae75171a344c66805e83c4bb306cb91e2b6f05eb2477838724fdef79571584d5b5577edf389b437db2e0e6008c4534b37f925aef1168c53bdbeaeb7538ca0f145db4d24e50af54b5019b7e201480c3869b11aad7ea786c130c9c14de99be26ab22fae57b5dff9d1159fe10188c0a5ed36a442422fd4e8805da3c6dbc16d5434ad2006638ec81904dc34a0f168b76874938ffcc7f1995388362c26259a6765b278895f33999e3d9ca4b4f64862cb99ae14ffb62e71ccf8ec722b65c1b334c44977bc99f5c7036f4fecc30249b5ee1f8ad444164a6c9055915c6454a09a4381874449dfaf9e76711c62e57ef4d247c77dae1cdd9329762b5ae694c7fb9ff8fc97a852e7b189a2423ff72c3c52782a98dea8d9a07a8481062a47762c771d5de93fd37416de39e9337f458ea682147a2f0359512e0d74ce5699563f2a93f88ea5be99e7e4948234e03a015d0c7eeba58e071e98cfeaf4d1dafa06171dc49f16df68cd2a2b280e53c588d91e298bb9de26408e6eebee1e5e55430947109c3c6b85d4919deab602f2b2e156565cbf8f833d7c208ab65f68c1fd4915f06c5034e05b027f0aec1907cca270b07db27aef70aa1f863341eac73f40602213cafa315eb7b04955e552efbdeb6e81b473977a04a92f723dc192b2c0a473b7c418e3fa7e9959be7eb1f801aabaea65e25b6e3035b9f4f6afb57d964d1bda864fad2733e645c11e602d430bd7061c1330b01c4313bc25c9c4b5605828f577f5aebd63482105344b9123781783d79a1e3a758482285b6cab6aabaa78cf1ada7767f717f2a3733de149909d997b3039c0de458bd3806267c63477ec77105266242a85f4127a03dd6fcaaff6e93089468e60c6fe8838c10b9db82c6810c9d11f63ecfb9c8679cedf70b58208274b9d35ac10c8ab85dca8cd0984487ddc923245a78d4c3359f6abb48d4540e7834e85fa290cc7ee12d188d94d816879015ef06383f9bb574fbf76e6ed2fefabe35f1f6ae41942b0229eed7c50df51a79f9566835e324b4450d1e7ec4ca9fc78450370cab0cb65795be6d7ac3e71e1bcab4f28d589ac0f99e0318fbbd96aa30d478a6204888851a66b3a9ebc85e1e9194d59d15a71658f5fac3ce1a0e99307aaebde657332fa0a2645c5c430b2900726fa245620ae90a7eb3f718f7d576c9d0381f97dad43c7a02644f66966ce68202724d141a5876b7b33dbf65c9ab80000c886128ba8a18f5563b08771979031bc9f8617288a3ce0a9cb1d8cdab427fd8389e16c11b6a7724473658e98b56fb9e7b88120a6dffe198d3b3b5225d0a05132705dc8db6400e1507cc29cdbb9a78412cdb8a4f6bf775000d189a277efddac02dd299e05e3255dba148$c9ed11dcd424349ff64092492e4ff7357ab4d239", "openwall123"},
	// above hash with kdf_type added
	{"$openbsd-softraid$8192$2fada40a4b317c3f8829abc2d046fa33fefc73dab99cc0f68577598ff5d673892145b48afe8eee76baaee531097bb60c6888e74097d56530738c6f572dc1a4e603c7e89631cfcfda6bb8bdcdf681960037c05c8e729b4ecd0c247b6cd504c27002cd8356deadb38c628104e307176218371afac51f7382b7dad8cfb7f65b509a$36ce839df6aaea0b274bed8e15783f57de169fe0361c924e0ad6466c6db52cf41fd07f3848b58e15c5e1647e1b9c513c83b4e09f46e3975d08ec615c74b2b722a7f6f230a94a127a278baaa15b911ad986e7ecc5026f3f012a12ad1b20c1490249ff83c3a782732017e7897e304f4c662b04d47b46b64abc11a98c953220f4210df74351d667a9a12d18883f4e309d1c694488c0cfa817afe354624333172b74d1f7e504d64643106d40cc35059276058b6d653faad6463458529890e109ec4313e3be4782ffb761d0444cf4096e54b663bb2cfe219aed10db7f2dc00aef69b34262cd0ae9c0a59f47c74665adf0a210e78e3b16d57aba9eba2bfc519648ac60941e0d736d6b30b52d8c6ae254127b6c156c5fa0388b3a90a592f26b8a9b66032ebaf2ca536fb8696b2dd4ae1d45488dd6c52bb94777ef2840fda049e0a7bb8b860f1a3b6ca5595076942ca985eefe4c1f97ddc6d16e48627f9f2e2aa31c965e67adf2c7594fcda50f8dde15bcb7045b0bae24b57da6f88154e268ffa68378ef424a5a2b988628c7630c59cd56157ea4e653614f8e55427e39d37d07b43647f8c1bafc6c2bc1c00cb37c13fa25e5205ef2f03b9613f8e4baafc48ad2d0b3feee9cd4807984e9e0650e9f639a45ce165e0478422d1e2aad9a3b056876be4b5219467149599dcc352fd8542ce28dc4f1350713b774c65e3bec345f1925a68afb71688fd683beb7a099f4ac13305abc4e2f40efb3c4147ca8b1b5b58cea0eeee71d2884e96e8c88214c86d326f56a6ede525bccff75784857fab35db71ab2cc6d06017fbc43a5e760cdf83608c47b28c98acfbff8a299ec222231aac7064768220414e829fc464f06b09bf0ef74f8eb356abeb253aaa3d6ee916bc19eacfff70fdc9b2f041bcacf107c8f4ccca06be2ec3fbc2c5734399f160e1f964feba8fce796f20c9c4899ca3e638d12c9a30d06f6cd19fb613be75146053dcfe008a6ff2324440695e70145b1c063fe594ad37f0197b4785cdd81f2de8776d8d7eafbe0bf3533c56dd3fd77456a087b17ce29a15b1df5b2129ba9ef54ffc19b67eb0c6c34024cc3f0fbbf3f81489d10a7d28348ceca62f28b040c9bc32fb9771fbf8ddb5ef34677c13f693318090d4d66b6f507817485ef438072892b10d20982ce778fe410699b4e4ab44647a2cf3af34d465784fbf22a387809adb60248d663994cf68d5ce12278ae3421c48874808c035a445cafcee5be55e03b1c744c12a29327e3ff2cb5e78f8ccb00a2bef88e122b1381a459351bd45260ffb7b6be334719551750c313adf3a9b777095386e78786837e05c4c56fb5093eef9ba8365a103c78e36755619e60ad6781d4d9d1c2b7ee7b471d4c41e525e08d3a60c98916005c13dbb0afe8c4ccf2160390e7a955f208fcf9d59cdaf28ef917b25f5158755288a6f52ad46bf96e7af2a02a35d582ae8c412810355f6973ee9f99d28112374bbf11dd7fd27f7ff48703b85b1167061555a89a66bfbe89f981ae75171a344c66805e83c4bb306cb91e2b6f05eb2477838724fdef79571584d5b5577edf389b437db2e0e6008c4534b37f925aef1168c53bdbeaeb7538ca0f145db4d24e50af54b5019b7e201480c3869b11aad7ea786c130c9c14de99be26ab22fae57b5dff9d1159fe10188c0a5ed36a442422fd4e8805da3c6dbc16d5434ad2006638ec81904dc34a0f168b76874938ffcc7f1995388362c26259a6765b278895f33999e3d9ca4b4f64862cb99ae14ffb62e71ccf8ec722b65c1b334c44977bc99f5c7036f4fecc30249b5ee1f8ad444164a6c9055915c6454a09a4381874449dfaf9e76711c62e57ef4d247c77dae1cdd9329762b5ae694c7fb9ff8fc97a852e7b189a2423ff72c3c52782a98dea8d9a07a8481062a47762c771d5de93fd37416de39e9337f458ea682147a2f0359512e0d74ce5699563f2a93f88ea5be99e7e4948234e03a015d0c7eeba58e071e98cfeaf4d1dafa06171dc49f16df68cd2a2b280e53c588d91e298bb9de26408e6eebee1e5e55430947109c3c6b85d4919deab602f2b2e156565cbf8f833d7c208ab65f68c1fd4915f06c5034e05b027f0aec1907cca270b07db27aef70aa1f863341eac73f40602213cafa315eb7b04955e552efbdeb6e81b473977a04a92f723dc192b2c0a473b7c418e3fa7e9959be7eb1f801aabaea65e25b6e3035b9f4f6afb57d964d1bda864fad2733e645c11e602d430bd7061c1330b01c4313bc25c9c4b5605828f577f5aebd63482105344b9123781783d79a1e3a758482285b6cab6aabaa78cf1ada7767f717f2a3733de149909d997b3039c0de458bd3806267c63477ec77105266242a85f4127a03dd6fcaaff6e93089468e60c6fe8838c10b9db82c6810c9d11f63ecfb9c8679cedf70b58208274b9d35ac10c8ab85dca8cd0984487ddc923245a78d4c3359f6abb48d4540e7834e85fa290cc7ee12d188d94d816879015ef06383f9bb574fbf76e6ed2fefabe35f1f6ae41942b0229eed7c50df51a79f9566835e324b4450d1e7ec4ca9fc78450370cab0cb65795be6d7ac3e71e1bcab4f28d589ac0f99e0318fbbd96aa30d478a6204888851a66b3a9ebc85e1e9194d59d15a71658f5fac3ce1a0e99307aaebde657332fa0a2645c5c430b2900726fa245620ae90a7eb3f718f7d576c9d0381f97dad43c7a02644f66966ce68202724d141a5876b7b33dbf65c9ab80000c886128ba8a18f5563b08771979031bc9f8617288a3ce0a9cb1d8cdab427fd8389e16c11b6a7724473658e98b56fb9e7b88120a6dffe198d3b3b5225d0a05132705dc8db6400e1507cc29cdbb9a78412cdb8a4f6bf775000d189a277efddac02dd299e05e3255dba148$c9ed11dcd424349ff64092492e4ff7357ab4d239$1", "openwall123"},
	// bcrypt PBKDF
	{"$openbsd-softraid$16$981b56db39bb572998affacafe76d495efd75212b0c31dccb4e8e8f70ecc874ddbc51d5cdd2b4fff6d98ee589cb271738b55f43c33e620eaef93e21398963421031940e455c44bcfbbcf0e686e2b860585bea1ab4891cf666a147ae2da97243d068a7171a1227a667cbbda83c50ff3ed9fdd447ed4d9699844a5b68863f3b3df$8fb40eb55b0ced789a88138f70c06c3c71321c49fc4a8dace25aaac36cdf6615e09896f8f3eba0b9d0f1cedb67935b39d45667abf976a0e032082eff80f4ab6bbc004ed29a3c54c49607ec76ca8135d08b63a144b1337e2345e82247955576221d4a59e2f247b0037599328cf0f7c4f45dc4fd5c0cf6b9ca788cadd9fa7f02386d67004477d69e19d1ebdf2d8b9eb529189dcb7c6093353465c055b8b9cec24e6a5f47191b4e3759fb8f9b2e7dab90678c336ea25f0e2310063462b4ef38c30c0bb31ae5b1b6a3b237df30b278407ce94aac718b0784d95b6fa1ac4c1f1d8f570485967144ef5081a0b156db126c73bb74766d99f8691aa327d55525533983333949ab2180a90064ed4673740a599860778fc99bed8b72d153ad9d365e81e1000d3952856538fcef36c4892605eb9a10cb1820769b0387cd8a6617a8e6774f572ce1c0208e70364bbbdbe16afc6e9caddaecd34cc2d4d2e9ca9a87285dcc560625320378be238faaa00d143d5294e284471d906ea56c7c2d2c3575d83e78c68da2eaf3b80bfb8ad7ab6b9265c62e8b07e9e285cd37dd4b1eccf4ae031cf6f6855db85cf2238aa88c1c101371c8b3dfb03db1d732a7777644c2942c14e44ce4eaa2894c5392ff2ee0c0f02cf0cd6d50b645648a7aa039287b6a748f27b325e8b7206c915dafef0b11166860475d67c921a637aaddb25a98812be7076f9a2c62071552444d6d59d24227d8bb948834ea3b0a14ea1b0e3682ae4667e47c271b11847889995cd0c8bed5740b0b4c4873d285cef1213777f1227d2447c43fa9f6410e6f76571fffb47d7594ac0bf1ecdf27d13543bbe59f99f4d87f02ef62ce2ba34cf090410d024395c178cf22d5852289fddcfc4ab4088beacc6efbc365013eedc9a7d43070122cd2bfc96a7eb4802bb672b613f3d11ea108ef7cd03985e46340d4cf9fb4e91a5782a4c0c013e1670af8b6fd7c6f4b55fd4234bea13cd6bf431d037a315e93787385eefd70e652fa9f86198e74816e203a2425b7c2378984ed3d795719d7c6c05478729ce9e863791e65a33de5cb71bf4d3e9ba7ec7990d0dfcfaaa2a2405a7bad1f267ccdc2116e32d4637ee10912f56dd4f4f6ea9131769c7d818098d3b4a555a8c5b6b4ba601c1e939871d1b2eda01ad1190e7f94817a82474badd634df8bba93ce02d6384aa704476914490e8b648bf1d71b623d03b97bc33661b6b744426da73cacf104af07eb49d1b0384a49635d1acd332a9047c39000b408f40b86767ada19cc87858134449fcbc43029a9c6192e127638c31b2093fa0c397303c35a0f1ea04a7206822ff05dd277e77d4ae99cffe13b4f56d13cfb54e5ae2af42ce24f1b7247fb91e14b3a829f1be11039978d84c71cde0156a00a042b8b337ccdb28fe7191e51f135b5278852ad749cdde3d90d024e1e20b35bc14fdfea745fe92dd35d13be1b61cf0b00f1d838f854b678d89f813bdc1d89e21fc5d9e4455f6655b8b2216e5826ceff8a2bcef2f0066525ad25384e21664187a6a1e5b9c38e04ab66bc557802ef29e93608cdabe78fe14f32e2c779e090ebce74ba472373857bb76795e81da34a0e637a5c8c4532ac1e0cfc6ca07a55c3f8a3c1d68e2225d48f29ab5da2c6034506bbe7507be4587781b780544bf15570689097ab78f09e2787183e0a6b1e068875cc228722a644d875f8e95f45e02e2f9178afc379c10a3c840eef997b76ec9a0860f0c0361ffd39bda157dacf54eb6197182edf6733b9ca3ef31f997389123f46c5d64a4fdcfadb1840e22a1ea7dddc2ab8be4dda87d4bfb543daa5af34d622b50432a354dde602910708c42661f29131d4d2dbcddb2447cc82e869f42e573d4b55ea21b6a6c8580a6b10a579ebf0b03cfe98a2e7d1e4eb492dcf79c937db4d386bae2b67f167f27d6771584a9a9466a7fc6856c39bde70dfd1451b49ea232f2e2d17fdcb11eec11d089c1ba0d0b11c21783ce42edda7dc9061e04098e81efb402f8f38f1a7c704843f2f865af40dc9e94b539979ba64696d7a284f2f46806decdc4b309aa562be7218e0706b3d075a170e3e9da0a6d8a28ea1afc5171f9ea324ed91cedf6a7fcf6222f65824571281eedf7423c42312c7b2152263cbef56100e8150c445cfac74b5af5c7ab36075fb8ac0b956c693245244fc4b00402323123158e9cdafb5f571090bb96b6cffb5de7ee37778f460c1a1616bef484ae69602e99a7d710b2f941c03849785dbcc85e90e0397b93765c17d097ad5c3de47699883316ab7ef9c3ae2f4a57d08d1ecc41e1d7753a8c04e735cbef995b7f44ac252d26d32c897e8e80965eb202bed3fae87b777f840d1f00cb788854e08f5eb4ab240c7d3ce2d41c532f19ad30f39ab7d1ce280ccb8bd5247d0c54cd066c11f5cbd9d78ac757719abe8f3bbdcc3a699d38ec9d32504f857c6cc8946e8ae67437a1c46e5e21fea38a91a5f357cb948feb892945926e23fd66d0ecfa6d0a5dc8a53ffa69230fda9adcd0cdfabfe56333a07ab711abe5ee78091c64ac2ea1418bf2f4f4f215ccab23123c553f82bc75324147cd0ce30393acf07bb355efca25272bba208d5fe7bd65317c3497d4e1ab551dbdd75fa6c0afaaab616983b5db9a343067a1c5f41a74c119d2635eade24e52cc6a5df7256256f00fb6e6b00075bd4570aafe4d382119d5d2aa90b9c6f170ca0a807678227bd953a6d5193a775e70c64d49a17512042fb9d1515be8bded24c25f63548670221598cb295a28dbd5a7bba55603cf0e928eb00aa1d3a10a00050885786b55bf4c80f786ee1d90e904940aa2f38c3d2ac806a2e119ee9dac6936645d2f81af80224060f23d8107f95d8cad4e55e39da0f66a7497150283508b5044bae24faa8bca6fe$c1459799ddcfb4f0da35506f7cb3bef58aceba2d$3", "openwall12345"},
	{NULL}
};

static char (*key_buffer)[PLAINTEXT_LENGTH + 1];
static uint32_t (*crypt_out)[BINARY_SIZE / sizeof(uint32_t)];

static struct custom_salt {
	unsigned int num_iterations;
	unsigned char salt[OPENBSD_SOFTRAID_SALTLENGTH];
	unsigned char masked_keys[OPENBSD_SOFTRAID_KEYLENGTH * OPENBSD_SOFTRAID_KEYS];
	int kdf_type;
} *cur_salt;

static void init(struct fmt_main *self)
{
#ifdef _OPENMP
	omp_t = omp_get_max_threads();
	self->params.min_keys_per_crypt *= omp_t;
	omp_t *= OMP_SCALE;
	self->params.max_keys_per_crypt *= omp_t;
#endif
	key_buffer = mem_calloc(sizeof(*key_buffer), self->params.max_keys_per_crypt);
	crypt_out = mem_calloc(sizeof(*crypt_out), self->params.max_keys_per_crypt);
}

static void done(void)
{
	MEM_FREE(crypt_out);
	MEM_FREE(key_buffer);
}

static int valid(char* ciphertext, struct fmt_main *self)
{
	char *ctcopy;
	char *keeptr;
	char *p;
	int kdf_type;

	if (strncmp(ciphertext, FORMAT_TAG, FORMAT_TAG_LEN) != 0)
		return 0;

	ctcopy = strdup(ciphertext);
	keeptr = ctcopy;
	ctcopy += FORMAT_TAG_LEN;

	if ((p = strtokm(ctcopy, "$")) == NULL)
		goto err;
	if (!isdec(p))            /* iterations */
		goto err;
	if ((p = strtokm(NULL, "$")) == NULL)
		goto err;
	if (strlen(p) != 2 * 128) /* salt */
		goto err;
	if (!ishexlc(p))
		goto err;
	if ((p = strtokm(NULL, "$")) == NULL)
		goto err;
	if (strlen(p) != 2 * 32 * 64) /* masked keys */
		goto err;
	if (!ishexlc(p))
		goto err;
	if ((p = strtokm(NULL, "$")) == NULL)
		goto err;
	if (strlen(p) != 2 * BINARY_SIZE) /* HMAC-SHA1 */
		goto err;
	if (!ishexlc(p))
		goto err;
	if ((p = strtokm(NULL, "$")) != NULL) { /* kdf type */
		if (strlen(p) != 1)
			goto err;
		if (!isdec(p))
			goto err;
		kdf_type = atoi(p);
		if (kdf_type !=1 && kdf_type != 3)
			goto err;
	}

	MEM_FREE(keeptr);
	return 1;

err:
	MEM_FREE(keeptr);
	return 0;
}

static void set_salt(void *salt)
{
	cur_salt = (struct custom_salt *)salt;
}

static void* get_salt(char *ciphertext)
{
	static struct custom_salt cs;
	char *ctcopy = strdup(ciphertext);
	char *keeptr = ctcopy;
	int i;
	char *p;

	ctcopy += FORMAT_TAG_LEN;
	p = strtokm(ctcopy, "$"); /* iterations */
	cs.num_iterations = atoi(p);
	p = strtokm(NULL, "$");   /* salt */
	for (i = 0; i < OPENBSD_SOFTRAID_SALTLENGTH ; i++)
		cs.salt[i] = atoi16[ARCH_INDEX(p[i * 2])] * 16
			+ atoi16[ARCH_INDEX(p[i * 2 + 1])];
	p = strtokm(NULL, "$");   /* masked keys */
	for (i = 0; i < OPENBSD_SOFTRAID_KEYLENGTH * OPENBSD_SOFTRAID_KEYS; i++)
		cs.masked_keys[i] = atoi16[ARCH_INDEX(p[i * 2])] * 16
			+ atoi16[ARCH_INDEX(p[i * 2 + 1])];
	p = strtokm(NULL, "$");   /* binary hash */
	p = strtokm(NULL, "$");   /* kdf type */
	if (p)
		cs.kdf_type = atoi(p);
	else
		cs.kdf_type = 1;

	MEM_FREE(keeptr);
	return (void *)&cs;
}

static void *get_binary(char *ciphertext)
{
	static union {
		unsigned char c[BINARY_SIZE];
		ARCH_WORD dummy;
	} buf;
	unsigned char *out = buf.c;
	char *p, *cc = NULL;
	int i;

	p = strrchr(ciphertext, '$') + 1;

	if (strlen(p) == 1) { // hack, last field is kdf type
		cc = strdup(ciphertext);
		cc[strlen(ciphertext) - 2] = 0;
		p = strrchr(cc, '$') + 1;

	}
	for (i = 0; i < BINARY_SIZE; i++) {
		out[i] = (atoi16[ARCH_INDEX(*p)] << 4) |
			atoi16[ARCH_INDEX(p[1])];
		p += 2;
	}
	if (cc)
		MEM_FREE(cc);

	return out;
}

static int crypt_all(int *pcount, struct db_salt *salt)
{
	const int count = *pcount;
	int index = 0;
#ifdef _OPENMP
#pragma omp parallel for
	for (index = 0; index < count; index += MAX_KEYS_PER_CRYPT)
#endif
	{
		AES_KEY akey;
		unsigned char mask_key[MAX_KEYS_PER_CRYPT][32];
		unsigned char unmasked_keys[OPENBSD_SOFTRAID_KEYLENGTH * OPENBSD_SOFTRAID_KEYS];
		unsigned char hashed_mask_key[20];
		int i, j;

		/* derive masking key from password */
		if (cur_salt->kdf_type == 1) {
#ifdef SSE_GROUP_SZ_SHA1
			int lens[SSE_GROUP_SZ_SHA1];
			unsigned char *pin[SSE_GROUP_SZ_SHA1], *pout[SSE_GROUP_SZ_SHA1];
			for (i = 0; i < SSE_GROUP_SZ_SHA1; ++i) {
				lens[i] = strlen(key_buffer[index+i]);
				pin[i] = (unsigned char*)key_buffer[index+i];
				pout[i] = mask_key[i];
			}
			pbkdf2_sha1_sse((const unsigned char **)pin, lens,
					cur_salt->salt, OPENBSD_SOFTRAID_SALTLENGTH,
					cur_salt->num_iterations, (unsigned char**)pout,
					32, 0);
#else
			for (i = 0; i < MAX_KEYS_PER_CRYPT; ++i) {
				pbkdf2_sha1((const unsigned char*)(key_buffer[index+i]),
						strlen(key_buffer[index+i]),
						cur_salt->salt, OPENBSD_SOFTRAID_SALTLENGTH,
						cur_salt->num_iterations, mask_key[i],
						32, 0);
			}
#endif
		} else if (cur_salt->kdf_type == 3) {
			for (i = 0; i < MAX_KEYS_PER_CRYPT; ++i) {
				bcrypt_pbkdf((const char*)key_buffer[index+i],
						strlen(key_buffer[index+i]),
						cur_salt->salt, OPENBSD_SOFTRAID_SALTLENGTH,
						mask_key[i], 32, cur_salt->num_iterations);
			}
		}

		for (i = 0; i < MAX_KEYS_PER_CRYPT; ++i) {
			/* decrypt sector keys */
			AES_set_decrypt_key(mask_key[i], 256, &akey);
			for (j = 0; j < (OPENBSD_SOFTRAID_KEYLENGTH * OPENBSD_SOFTRAID_KEYS) / 16;  j++) {
				AES_decrypt(&cur_salt->masked_keys[16*j], &unmasked_keys[16*j], &akey);
			}

			/* get SHA1 of mask_key */
			SHA1(mask_key[i], 32, hashed_mask_key);

			hmac_sha1(hashed_mask_key, OPENBSD_SOFTRAID_MACLENGTH,
					unmasked_keys, OPENBSD_SOFTRAID_KEYLENGTH * OPENBSD_SOFTRAID_KEYS,
					(unsigned char*)crypt_out[index+i], 20);
		}
	}

	return count;
}

static int cmp_all(void *binary, int count)
{
	int index = 0;
	for (; index < count; index++)
		if (*(uint32_t*)binary == *(uint32_t*)(crypt_out[index]))
			return 1;
	return 0;
}

static int cmp_one(void *binary, int index)
{
	return (*(uint32_t*)binary == *(uint32_t*)(crypt_out[index]));
}

static int cmp_exact(char *source, int index)
{
	void *bin = get_binary(source);
	return !memcmp(bin, crypt_out[index], 20);
}

static void jtr_set_key(char* key, int index)
{
	strcpy(key_buffer[index], key);
}

static char *get_key(int index)
{
	return key_buffer[index];
}
/* report iteration count as tunable cost */
static unsigned int iteration_count(void *salt)
{
	return ((struct custom_salt*)salt)->num_iterations;
}

struct fmt_main fmt_openbsd_softraid = {
	{
		FORMAT_LABEL,
		FORMAT_NAME,
		ALGORITHM_NAME,
		BENCHMARK_COMMENT,
		BENCHMARK_LENGTH,
		0,
		PLAINTEXT_LENGTH,
		BINARY_SIZE,
		BINARY_ALIGN,
		SALT_SIZE,
		SALT_ALIGN,
		MIN_KEYS_PER_CRYPT,
		MAX_KEYS_PER_CRYPT,
		FMT_CASE | FMT_8_BIT | FMT_OMP,
		{
			"iteration count",
		},
		{ FORMAT_TAG },
		tests_openbsdsoftraid
	}, {
		init,
		done,
		fmt_default_reset,
		fmt_default_prepare,
		valid,
		fmt_default_split,
		get_binary,
		get_salt,
		{
			iteration_count,
		},
		fmt_default_source,
		{
			fmt_default_binary_hash /* Not usable with $SOURCE_HASH$ */
		},
		fmt_default_salt_hash,
		NULL,
		set_salt,
		jtr_set_key,
		get_key,
		fmt_default_clear_keys,
		crypt_all,
		{
			fmt_default_get_hash /* Not usable with $SOURCE_HASH$ */
		},
		cmp_all,
		cmp_one,
		cmp_exact
	}
};

#endif
